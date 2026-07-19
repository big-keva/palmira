#include "http-server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "../docapi/docapi-req.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    std::uint16_t g_listen_port = 9200;
//-------------------------------------------------------------------------//
    // Validates a configuration on valid.
    auto validate_config(const mtc::config &config) -> void
    {
      if (config.to_zmap().get_zmap("ssl", {}).get_int32("enabled", 0) != 0)
      {
        const auto ssl = config.to_zmap().get_zmap("ssl", {});
        if (ssl.get_charstr("priv_key_file", "").empty())
        {
          throw(std::invalid_argument("ssl config file path is empty"));
        }

        if (ssl.get_charstr("cert_file", "").empty())
        {
          throw(std::invalid_argument("certificate file path is empty"));
        }

        if (ssl.get_charstr("dh_params_file", "").empty())
        {
          throw(std::invalid_argument("dh_params file path is empty"));
        }
      }
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  HttpServer::HttpServer(palmira::IService *srv, const mtc::config &cfg)
      : config(cfg), service(srv),
        executer(std::max(1U, config.get_int32("workers", 1) == 0
          ? std::thread::hardware_concurrency() - 1
          : config.get_int32("workers", 1)))
  {
    if (not this->config.has_key("port"))
    {
      std::fprintf(stderr, "[WARN] Listening port not found. By default: %d\n", g_listen_port);
    }
    g_listen_port = this->config.get_int32("port", g_listen_port);

    // Validating a configuration parameters.
    validate_config(this->config);
  }

  HttpServer::~HttpServer()
  {
    this->HttpServer::Stop();
    this->HttpServer::Wait();
  }
//-------------------------------------------------------------------------//
  void HttpServer::Start()
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

  void HttpServer::Stop()
  {
    if (this->stopping.exchange(true))
    {
      return;
    }

    this->executer.stop();

    if (this->loop != nullptr)
    {
      this->loop->defer([this]() {
        if (this->listen_socket != nullptr)
        {
          us_listen_socket_close(0, listen_socket);
          this->listen_socket = nullptr;
        }
      });
    }

    std::unique_lock sync(this->mtx);
    this->cv.wait(sync, [this]() -> bool {
      return this->listen_socket != nullptr || this->start_failed;
    });
  }

  void HttpServer::Wait()
  {
    if (this->thread.joinable())
    {
      this->thread.join();
    }
  }
//-------------------------------------------------------------------------//
  auto HttpServer::onloop() -> void
  {
    this->loop = uWS::Loop::get();

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
          .dh_params_file_name = ssl.get_charstr("dh_params_file", "").c_str()
        });

        // Registering routes.
        this->register_routes<true>(app);

        // Listening the server.
        app.listen(listen_host, getListenPort(), [this](auto *token)
        {
          this->onlisten(token);
        });

        std::fprintf(stdout, "Module [%s] listening on https://%s:%d\n",
                     module_name.c_str(), listen_host.c_str(), getListenPort());
        // Executing server.
        app.run();
      }
      else
      {
        uWS::App app;
        // Registering routes.
        this->register_routes<false>(app);

        // Listening the server.
        app.listen(listen_host, getListenPort(), [this](auto *token) {
          this->onlisten(token);
        });

        std::fprintf(stdout, "Module [%s] listening on http://%s:%d\n",
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

  auto HttpServer::onlisten(us_listen_socket_t *token) -> void
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
//-------------------------------------------------------------------------//
} // namespace elastic
