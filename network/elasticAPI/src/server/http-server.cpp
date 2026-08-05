#include "http-server.h"
//-------------------------------------------------------------------------//
#include "../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../http/http-resp.h"
//-------------------------------------------------------------------------//
#include "../docapi/http/docapi-req.h"
#include "../docapi/http/routes.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    // Keeps listening port.
    std::uint16_t g_listen_port = 9200;

    // Keeps a global pool of threads.
    std::unique_ptr<thread_pool> g_thread_pool;

    // Keeps uWS event loop.
    uWS::Loop *g_ws_loop = nullptr;
//-------------------------------------------------------------------------//
    // Validates a configuration on valid.
    auto validate_config(const mtc::config &config) -> void
    {
      if (config.to_zmap().get_zmap("ssl", {}).get_int32("enabled", 0) != 0)
      {
        const auto ssl = config.to_zmap().get_zmap("ssl", {});
        if (ssl.get_charstr("priv_key_file", "").empty())
        {
          throw (std::invalid_argument("ssl config file path is empty"));
        }

        if (ssl.get_charstr("cert_file", "").empty())
        {
          throw (std::invalid_argument("certificate file path is empty"));
        }

        if (ssl.get_charstr("dh_params_file", "").empty())
        {
          throw (std::invalid_argument("dh_params file path is empty"));
        }
      }
    }
//-------------------------------------------------------------------------//
    template<typename app_t>
    auto register_default_route(app_t &app) -> void
    {
      app.get("/", [](auto *resp, auto *) {
        auto state = std::make_shared<async_response_state>();

        resp->onAborted([state]() {
          state->aborted.store(true, std::memory_order_release);
        });

        // Replying default route.
        http::send_default_response(resp);
      });

      app.any("/*", [](auto *resp, auto* /* request */) {
        auto state = std::make_shared<async_response_state>();

        resp->onAborted([state]() {
          state->aborted.store(true, std::memory_order_release);
        });

        http::send_default_response(resp)->writeStatus("404 Not Found")
                                         ->writeHeader("Content-Type", "application/json")
                                         ->end(R"({"error":"route not found","status":404})");
      });
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  http_server::http_server(palmira::IService *srv, const mtc::config &cfg)
    : config(cfg), service(srv),
      routes{
        docapi::http::routes{srv, cfg}
      }
  {
    if (not this->config.has_key("port"))
    {
      LOG_W_C("[WARN] Listening port not found. By default: %d", g_listen_port);
    }
    g_listen_port = this->config.get_int32("port", g_listen_port);

    // Validating a configuration parameters.
    validate_config(this->config);

    if (this->config.has_key("logger"))
    {
      const auto &logger = this->config.get_section("logger");

      // Initializing a logger.
      logger::init_logger(logger.get_path("file"));

      // Setting debug level.
      logger::set_debug_level(logger.get_charstr("debug_level", "info").c_str());
    }

    // Initializing a pool of thread.
    g_thread_pool = std::make_unique<thread_pool>(std::max(1U, config.get_int32("workers", 1) == 0
      ? std::thread::hardware_concurrency() - 1 : config.get_int32("workers", 1)));
  }

  http_server::~http_server()
  {
    this->http_server::Stop();
    this->http_server::Wait();

    g_thread_pool.reset();

    // Destroying a logger.
    logger::destroy_logger();
  }
//-------------------------------------------------------------------------//
  void http_server::Start()
  {
    bool expected = false;

    if (not this->started.compare_exchange_strong(expected, true))
    {
      return;
    }

    this->thread = std::thread([this]() {
      this->onloop();
    });
  }

  void http_server::Stop()
  {
    if (this->stopping.exchange(true))
    {
      return;
    }

    g_thread_pool->stop();

    if (g_ws_loop != nullptr)
    {
      g_ws_loop->defer([this]() {
        if (this->listen_socket != nullptr)
        {
          us_listen_socket_close(0, this->listen_socket);
          this->listen_socket = nullptr;
        }
      });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  void http_server::Wait()
  {
    if (this->thread.joinable())
    {
      this->thread.join();
    }
  }
//-------------------------------------------------------------------------//
  template<typename app_t>
  auto http_server::register_routes(app_t &app) -> void
  {
    std::apply([&app](auto&... module) {
      (
        module.register_routes(app),
        ...
      );
    },
    this->routes);

    // Registering default route.
    register_default_route(app);
  }
//-------------------------------------------------------------------------//
  auto http_server::onloop() -> void
  {
    g_ws_loop = uWS::Loop::get();

    try
    {
      const auto listen_host = this->config.get_charstr("listen_address", "0.0.0.0");
      const auto module_file = this->config.get_charstr("path", "");
      const auto module_name = this->config.get_charstr("name", "ElasticSearch");

      if (this->config.to_zmap().get_zmap("ssl", {}).get_int32("enabled", 0) != 0)
      {
        const auto &ssl = this->config.to_zmap().get_zmap("ssl", {});
        uWS::SSLApp app({
          .key_file_name = ssl.get_charstr("priv_key_file", "").c_str(),
          .cert_file_name = ssl.get_charstr("cert_file", "").c_str(),
          .passphrase = ssl.get_charstr("passphrase", "").c_str(),
          .dh_params_file_name = ssl.get_charstr("dh_params_file", "").c_str(),
          .ca_file_name = ssl.get_charstr("ca_file_name", "").c_str(),
          .ssl_ciphers = ssl.get_charstr("ssl_ciphers", "").c_str(),
          .ssl_prefer_low_memory_usage = ssl.get_bool("low_memory_usage", false),
        });

        // Registering routes.
        this->register_routes(app);

        // Listening the server.
        app.listen(listen_host, getListenPort(), [this](auto *token) {
          this->onlisten(token);
        });

        LOG_I_C("Module [%s] listening on https://%s:%d",
                module_name.c_str(), listen_host.c_str(), getListenPort());
        // Executing server.
        app.run();
      }
      else
      {
        uWS::App app;
        // Registering routes.
        this->register_routes(app);

        // Listening the server.
        app.listen(listen_host, getListenPort(), [this](auto *token) {
          this->onlisten(token);
        });

        LOG_I_C("Module [%s] listening on http://%s:%d",
                module_name.c_str(), listen_host.c_str(), getListenPort());
        // Executing server.
        app.run();
      }
    }
    catch (...)
    {
      std::lock_guard sync(this->mtx);
      this->start_failed = true;
      this->cv.notify_all();
    }
  }

  auto http_server::onlisten(us_listen_socket_t *token) -> void
  {
    {
      std::lock_guard sync(this->mtx);
      this->listen_socket = token;
      this->start_failed = token == nullptr;
    }
    this->cv.notify_all();
  }
//-------------------------------------------------------------------------//
  auto getListenPort() -> std::uint16_t
  {
    return g_listen_port;
  }

  auto get_thread_pool() -> thread_pool *
  {
    return g_thread_pool.get();
  }

  auto get_ws_loop() -> uWS::Loop *
  {
    return g_ws_loop;
  }
//-------------------------------------------------------------------------//
} // namespace elastic
