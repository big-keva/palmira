#include "http-server.h"
//-------------------------------------------------------------------------//
#include "../docapi/docapi-req.h"
//-------------------------------------------------------------------------//
#include "../logger/logger.h"
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
  }

  HttpServer::~HttpServer()
  {
    this->HttpServer::Stop();
    this->HttpServer::Wait();

    // Destroying a logger.
    logger::destroy_logger();
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
          us_listen_socket_close(0, this->listen_socket);
          this->listen_socket = nullptr;
        }
      });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  void HttpServer::Wait()
  {
    if (this->thread.joinable())
    {
      this->thread.join();
    }
  }
//-------------------------------------------------------------------------//
  template<bool SSL>
  auto HttpServer::register_routes(uWS::TemplatedApp<SSL> &app) -> void
  {
    {// Document API
      // Adds a new document and automatically generates a unique ID.
      app.post("/:index/_doc", [this](auto *res, auto *req) {
        this->onpost<SSL>(res, req);
      });
      // Adds a new document with a specified ID or updates an existing document
      // with the same ID.
      app.put("/:index/_doc/:id", [this](auto *res, auto *req) {
        this->onput<SSL>(res, req);
      });

      // Adding a new document with a specified ID only if a document with that ID
      // does not already exist. If the document exists, the operation fails.
      app.post("/:index/_create/:id", [this](auto *res, auto *req) {
        this->onpost<SSL>(res, req);
      });
      app.put("/:index/_create/:id", [this](auto *res, auto *req) {
        this->onput<SSL>(res, req);
      });

      // Getting a document.
      app.get("/:index/_doc/:id", [this](auto *res, auto *req) {
        this->onget<SSL>(res, req);
      });

      app.get("/:index/_source/:id", [this](auto *res, auto *req) {
        this->onget<SSL>(res, req);
      });

      // Deleting a document.
      app.del("/:index/_doc/:id", [this](auto *res, auto *req) {
        this->ondel<SSL>(res, req);
      });
    }
  }
//-------------------------------------------------------------------------//
  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onpost(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<elastic::http::response_context<SSL>>(res);
    auto payload = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_D_C("Received POST request: index=%s, id=%s", index.c_str(), id.c_str());
    // Reserving resources.
    payload->reserve(body_size);

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    res->onData([this, resp_ctx, state, index, id, params, payload, body_size, started = request_started.time_since_epoch().count()](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      // Forwarding a request to search engine.
      this->executer.enqueue(executer_types::insert, [loop = this->loop, service = this->service, resp_ctx, index, id, body = std::move(*payload), params, timeout, &state, started](const executer_context &ctx) mutable -> void {
          try
          {
            LOG_D_C("Received POST request: index=%s, id=%s, payload=%s", index.c_str(), id.c_str(), body.c_str());
            // Parsing insert request.
            auto args = json::docapi::parse_index_request(std::string_view(body), mtc::zmap{
              {"_index", index},
              {"_id", id},
              {"_params", params},
              {"_started", started}
            });

            // Sending a document to search engine.
            service->Insert(args, [&ctx, &loop, resp_ctx, &state](const mtc::zmap &resp) {
              auto reply = http::service_response{
                .status = http::status_codes::OK,
                .content_type = "application/json; charset=utf-8",
                .body = ""};

              // Making a response.
              reply.body = http::docapi::make_index_response(resp);

              loop->defer([resp_ctx, state, reply = std::move(reply)]() mutable {
                  if (state->aborted.load(std::memory_order_acquire))
                  {
                    return;
                  }

                  // Replying a response to client.
                  http::send_json_response(resp_ctx.get(), reply);
                });
            });
          }
          catch (const elastic::json::parse_error &exc)
          {
            LOG_E_C("Proceeding POST request failed: %s", exc.what());
            http::send_error_response(resp_ctx.get(), http::status_codes::BAD_REQUEST, "payload_bad_request", exc.what());
          }
          catch (const std::exception &exc)
          {
            LOG_E_C("Proceeding POST request failed: %s", exc.what());
            http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
          }
          catch (...)
          {
            LOG_E_C("Proceeding POST request failed: %unknown");
            http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
          }
        });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onput(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);
    auto payload = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (id.empty())
    { // This ID is required
      state->aborted.store(true, std::memory_order_release);

      // Replying error message.
      http::send_error_response(resp_ctx.get(), http::status_codes::BAD_REQUEST, "pad_request", "document id is empty");
    }

    // Reserving resources.
    payload->reserve(body_size);

    res->onData([this, resp_ctx, state, index, id, params, started = request_started.time_since_epoch().count(), payload, body_size](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      // Forwarding a request to search engine.
      this->executer.enqueue(executer_types::insert, [loop = this->loop, service = this->service, resp_ctx, index, id, payload, params, timeout, state, started](const executer_context &ctx) mutable -> void {
        try
        {
          LOG_D_C("Received PUT request: index=%s, id=%s, payload=%s", index.c_str(), id.c_str(), payload->c_str());
          // Parsing insert request.
          auto args = json::docapi::parse_index_request(std::string_view(*payload), mtc::zmap{
            {"_index",   index},
            {"_id",      id},
            {"_params",  params},
            {"_started", started}
          });

          // Sending a document to search engine.
          auto resp = service->Insert(args, [&ctx, &loop, resp_ctx, &state](const mtc::zmap &resp) {
            LOG_T_C("Received PUT response: %s", mtc::to_string(resp).c_str());
            auto reply = http::service_response{
              .status = http::status_codes::OK,
              .content_type = "application/json; charset=utf-8",
              .body = ""
            };

            // Making a response.
            reply.body = http::docapi::make_index_response(resp);

            loop->defer([resp_ctx, state, reply = std::move(reply)]() mutable {
              if (state->aborted.load(std::memory_order_acquire))
              {
                return;
              }

              // Replying a response to client.
              http::send_json_response(resp_ctx.get(), reply);
            });
          });
        }
        catch (const elastic::json::parse_error &exc)
        {
          LOG_E_C("Proceeding PUT request failed: %s", exc.what());
          http::send_error_response(resp_ctx.get(), http::status_codes::BAD_REQUEST, "payload_bad_request", exc.what());
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding PUT request failed: %s", exc.what());
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding PUT request failed: unknown");
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onget(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);

    const auto params = http::parse_query(req->getQuery());
    const auto routing = http::get_query_param(params, "routing");
    const auto index = std::string(req->getParameter(0));
    const auto id = std::string(req->getParameter(1));
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_T_C("Received GET request: index=%s, id=%s", index.c_str(), id.c_str());
    res->onAborted([resp_ctx]() {
      resp_ctx->aborted.store(true, std::memory_order_release);
    });
    auto payload = std::make_shared<std::string>();

    // Reserving resources.
    payload->reserve(body_size);

    // Reading payload.
    res->onData([this, resp_ctx, index, id, params, routing, state, payload, body_size, request_started](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);
      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }
      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::PAYLOAD_TOO_LARGE, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      //<!!!> uWebSockets response lives in original event-loop inside.
      this->executer.enqueue(executer_types::search, [service = this->service, resp_ctx, index, id, params, routing, payload, started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()](const executer_context &ctx) mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          // Parsing a search request.
          const auto req = payload != nullptr && not payload->empty() ? json::docapi::parse_mget_request(*payload, index)
                                                                                 : json::docapi::parse_mget_request(index, id, mtc::zmap{
                                                                                    {"_index", index},
                                                                                    {"_id", id},
                                                                                    {"_routing", routing.has_value() ? routing.value() : ""},
                                                                                    {"_source", {}}
                                                                                   });

          palmira::SearchArgs args;
          // Setting an order of searching.
          args.order = mtc::zmap{
            {"first", 1},
            {"count", 10},
            {"quote", mtc::zmap{
              {"mode", "source"}
            }}
          };

          args.query = mtc::zmap{
            {"get", mtc::array_charstr{id}}
          };

/*<TODO> Adding support a list of docs.
          mtc::array_zmap docs;
          for (const auto &doc : req.docs)
          {
            docs.push_back({{"id", id}});
          }
          // Applying a list of docs.
          args.query = docs;
*/

          // Forwarding a search request into search engine.
          service->Search(args, [resp_ctx, index, id, ctx, state, loop](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__

            loop->defer([resp_ctx, resp, state, index, id]() mutable {
              if (state->aborted.load(std::memory_order_acquire))
              {
                return;
              }

              LOG_T_C("Replying a response: index=%s id=%s", index.data(), id.data());
              // Replying a response to client.
              http::docapi::send_json_response(resp_ctx.get(), resp, index, id);
            });
          });
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding GET request failed: %s", exc.what());
          // Replying to error response.
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding GET request failed: unknown");
          // Replying to error response.
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::ondel(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);

    const auto params = http::parse_query(req->getQuery());
    const auto routing = http::get_query_param(params, "routing");
    const auto index = std::string(req->getParameter(0));
    const auto id = std::string(req->getParameter(1));
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_T_C("Received DELETE request: index=%s, id=%s", index.c_str(), id.c_str());
    res->onAborted([resp_ctx]() {
      resp_ctx->aborted.store(true, std::memory_order_release);
    });
    auto payload = std::make_shared<std::string>();

    // Reserving resources.
    payload->reserve(body_size);

    // Reading payload.
    res->onData([this, resp_ctx, index, id, params, routing, state, payload, body_size, request_started](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);
      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }
      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::PAYLOAD_TOO_LARGE, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      //<!!!> uWebSockets response lives in original event-loop inside.
      this->executer.enqueue(executer_types::remove, [service = this->service, resp_ctx, index, id, params, routing, payload, started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()](const executer_context &ctx) mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          palmira::RemoveArgs args;
          args.objectId = id;

          // Forwarding a search request into search engine.
          service->Remove(args, [resp_ctx, index, id, ctx, state, loop](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received DELETE response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__

            loop->defer([resp_ctx, resp, state, index, id]() mutable {
              if (state->aborted.load(std::memory_order_acquire))
              {
                return;
              }

              LOG_T_C("Replying a response: index=%s id=%s", index.data(), id.data());
              // Replying a response to client.
              http::docapi::send_json_response(resp_ctx.get(), resp, index, id);
            });
          });
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding GET request failed: %s", exc.what());
          // Replying to error response.
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding GET request failed: unknown");
          // Replying to error response.
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
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
