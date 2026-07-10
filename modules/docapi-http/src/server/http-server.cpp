#include "http-server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "../http/docreq.h"
//-------------------------------------------------------------------------//
namespace docapi
{
//-------------------------------------------------------------------------//
namespace
{
std::uint16_t g_listen_port = 9200;

// Validates a configuration on valid.
auto validate_config(const mtc::zmap &settings) -> void
{
  if (settings.get_zmap("ssl", {}).get_int32("enabled", 0) != 0)
  {
    const auto ssl = settings.get_zmap("ssl", {});
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
} // namespace
//-------------------------------------------------------------------------//
HttpServer::HttpServer(mtc::api<palmira::IService> srv, mtc::zmap cfg)
    : settings(std::move(cfg)), service(std::move(srv)),
      executer(std::max(1U, settings.get_int32("workers", 1) == 0
                                ? std::thread::hardware_concurrency() - 1
                                : settings.get_int32("workers", 1)))
{
  if (this->settings.get_int32("listen_port", 0) > 0)
  {
    g_listen_port = this->settings.get_int32("listen_port", g_listen_port);
  }

  // Validating a configuration parameters.
  validate_config(this->settings);
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

  this->thread = std::thread([this]() { this->onloop(); });
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
    this->loop->defer([this]()
    {
      if (this->listen_socket != nullptr)
      {
        us_listen_socket_close(0, listen_socket);
        this->listen_socket = nullptr;
      }
    });
  }

  std::unique_lock sync(this->mtx);
  this->cv.wait(sync, [this]()
  {
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
    const auto listen_host = this->settings.get_charstr("listen_address", "0.0.0.0");
    const auto module_file = this->settings.get_charstr("file", "");
    const auto module_name = this->settings.get_charstr("name", module_file);

    if (this->settings.get_zmap("ssl", {}).get_int32("enabled", 0) != 0)
    {
      const auto &ssl = this->settings.get_zmap("ssl", {});
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
      app.listen(listen_host, getListenPort(), [this](auto *token)
      {
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
} // namespace docapi
