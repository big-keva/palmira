#include "routes.h"
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "docapi-req.h"
#include "docapi-resp.h"
//-------------------------------------------------------------------------//
#include "../../common/thread-pool.h"
//-------------------------------------------------------------------------//
#include "../../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../../http/http-req.h"
#include "../../http/http-resp.h"
//-------------------------------------------------------------------------//
#include "../../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
#include "../json/simd-json-index-parser.h"
#include "../json/simd-json-update-parser.h"
#include "../json/simd-json-mget-parser.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  auto get_thread_pool() -> elastic::thread_pool *;
  auto get_ws_loop() -> uWS::Loop *;
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
namespace elastic::docapi::http
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    // Keeps websocket loop.
    uWS::Loop *g_ws_loop = nullptr;

    // Keeps a global pool of threads.
    thread_pool *g_thread_pool = nullptr;
//-------------------------------------------------------------------------//
  struct async_response_state final
  {
    std::atomic_bool aborted{false};
  };
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  routes::routes(palmira::IService *srv, const mtc::config &cfg)
    : service(srv), config(cfg)
  {
  }
//-------------------------------------------------------------------------//
  template<typename Response, typename Request>
  auto routes::onpost(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    // auto resp_ctx = std::make_shared<elastic::http::response_context<SSL>>(res);
    auto payload = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = elastic::http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_D_C("Received POST request: index=%s, id=%s", index.c_str(), id.c_str());
    // Reserving resources.
    payload->reserve(body_size);

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (g_thread_pool == nullptr)
    {
      g_thread_pool = get_thread_pool();
    }
    res->onData([this, res, state, index, id, params, payload, body_size, started = request_started.time_since_epoch().count()](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");
      // Forwarding a request to search engine.
      g_thread_pool->enqueue([service = this->service, res, index, id, body = std::move(*payload), params, timeout, &state, started]() mutable -> void {
          try
          {
            LOG_D_C("Received POST request: index=%s, id=%s, payload=%s", index.c_str(), id.c_str(), body.c_str());
            // Parsing insert request.
            auto args = json::parse_index_request(std::string_view(body), mtc::zmap{
              {"_index", index},
              {"_id", id},
              {"_params", params},
              {"_started", started}
            });

            // Sending a document to search engine.
            service->Insert(args, [res, state, index, id](const mtc::zmap &resp) {
#ifdef __DEBUG__
              mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
              LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__
              if (g_ws_loop != nullptr)
              {
                g_ws_loop->defer([res, resp, state, index, id]() mutable {
                  if (state->aborted.load(std::memory_order_acquire))
                  {
                    return;
                  }

                  LOG_T_C("Replying a response: index=%s, id=%s, status=%s", index.data(), id.data(), mtc::to_string(resp).c_str());
                  // Replying a response to client.
                  http::send_index_json_response(res, resp, index, id);
                });
              }
            });
          }
          catch (const elastic::json::parse_error &exc)
          {
            LOG_E_C("Proceeding POST request failed: %s", exc.what());
            elastic::http::send_error_response(res, elastic::http::status_codes::BAD_REQUEST, "payload_bad_request", exc.what());
          }
          catch (const std::exception &exc)
          {
            LOG_E_C("Proceeding POST request failed: %s", exc.what());
            elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
          }
          catch (...)
          {
            LOG_E_C("Proceeding POST request failed: %unknown");
            elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
          }
        });
    });
  }

  template<typename Response, typename Request>
  auto routes::onput(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto payload = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = elastic::http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (id.empty())
    { // This ID is required
      state->aborted.store(true, std::memory_order_release);

      // Replying error message.
      elastic::http::send_error_response(res, elastic::http::status_codes::BAD_REQUEST, "pad_request", "document id is empty");
    }

    // Reserving resources.
    payload->reserve(body_size);

    res->onData([this, res, state, index, id, params, started = request_started.time_since_epoch().count(), payload, body_size](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");
      // Forwarding a request to search engine.
      g_thread_pool->enqueue([service = this->service, res, index, id, payload, params, timeout, state, started]() mutable -> void {
        try
        {
          LOG_D_C("Received PUT request: index=%s, id=%s, payload=%s", index.c_str(), id.c_str(), payload->c_str());
          // Parsing insert request.
          auto args = json::parse_index_request(std::string_view(*payload), mtc::zmap{
            {"_index",   index},
            {"_id",      id},
            {"_params",  params},
            {"_started", started}
          });
          // Sending a document to search engine.
          auto resp = service->Insert(args, [res, state, index, id](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__

            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, resp, state, index, id]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }
                LOG_T_C("Replying a response: index=%s, id=%s, status=%s", index.data(), id.data(), mtc::to_string(resp).c_str());
                // Replying a response to client.
                docapi::http::send_index_json_response(res, resp, index, id);
              });
            }
          });
        }
        catch (const elastic::json::parse_error &exc)
        {
          LOG_E_C("Proceeding PUT request failed: %s", exc.what());
          elastic::http::send_error_response(res, elastic::http::status_codes::BAD_REQUEST, "payload_bad_request", exc.what());
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding PUT request failed: %s", exc.what());
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding PUT request failed: unknown");
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<typename Response, typename Request>
  auto routes::onupdate(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto payload = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = elastic::http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (id.empty())
    { // This ID is required
      state->aborted.store(true, std::memory_order_release);

      // Replying error message.
      elastic::http::send_error_response(res, elastic::http::status_codes::BAD_REQUEST, "pad_request", "document id is empty");
    }

    // Reserving resources.
    payload->reserve(body_size);

    res->onData([this, res, state, index, id, params, started = request_started.time_since_epoch().count(), payload, body_size](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");
      // Forwarding a request to search engine.
      g_thread_pool->enqueue([service = this->service, res, index, id, payload, params, timeout, state, started]() mutable -> void {
        try
        {
          LOG_D_C("Received UPDATE request: index=%s, id=%s, payload=%s", index.c_str(), id.c_str(), payload->c_str());
          // Parsing insert request.
          auto args = json::parse_update_request(std::string_view(*payload), mtc::zmap{
            {"_index",   index},
            {"_id",      id},
            {"_params",  params},
            {"_started", started}
          });

          // Sending a document to search engine.
          //<???> auto resp = service->Update(args, [res, state, index, id](const mtc::zmap &resp) {
          auto resp = service->Insert(args, [res, state, index, id](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received UPDATE response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__

            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, resp, state, index, id]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }
                LOG_T_C("Replying a response: index=%s, id=%s, status=%s", index.data(), id.data(), mtc::to_string(resp).c_str());
                // Replying a response to client.
                http::send_update_json_response(res, resp, index, id);
              });
            }
          });
        }
        catch (const elastic::json::parse_error &exc)
        {
          LOG_E_C("Proceeding UPDATE request failed: %s", exc.what());
          elastic::http::send_error_response(res, elastic::http::status_codes::BAD_REQUEST, "payload_bad_request", exc.what());
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding UPDATE request failed: %s", exc.what());
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding UPDATE request failed: unknown");
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<typename Response, typename Request>
  auto routes::onget(Response *res, Request *req) -> void
  {
    auto payload = std::make_shared<std::string>();
    auto state = std::make_shared<async_response_state>();

    const auto params = elastic::http::parse_query(req->getQuery());
    const auto routing = elastic::http::get_query_param(params, "routing");
    const auto index = std::string(req->getParameter(0));
    const auto id = std::string(req->getParameter(1));
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_T_C("Received GET request: index=%s, id=%s", index.c_str(), id.c_str());
    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    // Reserving resources.
    payload->reserve(body_size);

    // Reading payload.
    res->onData([this, res, index, id, params, routing, state, payload, body_size, request_started](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);
      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }
      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        elastic::http::send_error_response(res, elastic::http::status_codes::PAYLOAD_TOO_LARGE, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");
      //<!!!> uWebSockets response lives in original event-loop inside.
      g_thread_pool->enqueue([service = this->service, res, index, id, params, routing, payload, started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()]() mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          // Parsing a search request.
          const auto req = payload != nullptr && not payload->empty() ? json::parse_mget_request(*payload, index)
                                                                                 : json::parse_mget_request(index, id, mtc::zmap{
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
            {"get", mtc::zmap{
              {"id", id}
            }}
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
          service->Search(args, [res, index, id, state](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__
            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, resp, state, index, id]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }

                LOG_T_C("Replying a response: index=%s, id=%s, status=%s", index.data(), id.data(), mtc::to_string(resp).c_str());
                // Replying a response to client.
                http::send_get_json_response(res, resp, index, id);
              });
            }
          });
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding GET request failed: %s", exc.what());
          // Replying to error response.
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding GET request failed: unknown");
          // Replying to error response.
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<typename Response, typename Request>
  auto routes::ondel(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();

    const auto params = elastic::http::parse_query(req->getQuery());
    const auto routing = elastic::http::get_query_param(params, "routing");
    const auto index = std::string(req->getParameter(0));
    const auto id = std::string(req->getParameter(1));
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_T_C("Received DELETE request: index=%s, id=%s", index.c_str(), id.c_str());
    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    auto payload = std::make_shared<std::string>();
    // Reserving resources.
    payload->reserve(body_size);

    // Reading payload.
    res->onData([this, res, index, id, params, routing, state, payload, body_size, request_started](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);
      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }
      if (payload->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        elastic::http::send_error_response(res, elastic::http::status_codes::PAYLOAD_TOO_LARGE, "payload_too_large", "request body is too large");
        return;
      }
      payload->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");
      //<!!!> uWebSockets response lives in original event-loop inside.
      g_thread_pool->enqueue([service = this->service, res, index, id, params, routing, payload, started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()]() mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          palmira::RemoveArgs args;
          args.objectId = id;

          // Forwarding a search request into search engine.
          service->Remove(args, [res, index, id, state](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received DELETE response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__
            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, resp, state, index, id]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }

                LOG_T_C("Replying a response: index=%s, id=%s, status=%s", index.data(), id.data(), mtc::to_string(resp).c_str());
                // Replying a response to client.
                http::send_del_json_response(res, resp, index, id);
              });
            }
          });
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding GET request failed: %s", exc.what());
          // Replying to error response.
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding GET request failed: unknown");
          // Replying to error response.
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }
//-------------------------------------------------------------------------//
  template<typename app_t>
  auto routes::register_routes(app_t &app) -> void
  {
    // Saving uWS event loop.
    g_ws_loop = get_ws_loop();
    assert(g_ws_loop != nullptr && "Invalid pointer on uWS event loop");

    g_thread_pool = get_thread_pool();
    assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");

    // Adds a new document and automatically generates a unique ID.
    app.post("/:index/_doc", [this](auto *res, auto *req) {
      this->onpost(res, req);
    });
    // Adds a new document with a specified ID or updates an existing document
    // with the same ID.
    app.put("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onput(res, req);
    });

    // Adding a new document with a specified ID only if a document with that ID
    // does not already exist. If the document exists, the operation fails.
    app.post("/:index/_create/:id", [this](auto *res, auto *req) {
      this->onpost(res, req);
    });
    app.put("/:index/_create/:id", [this](auto *res, auto *req) {
      this->onput(res, req);
    });

    // Updating document.
    app.post("/:index/_update/:id", [this](auto *res, auto *req) {
      this->onupdate(res, req);
    });

    // Getting a document.
    app.get("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onget(res, req);
    });

    app.get("/:index/_source/:id", [this](auto *res, auto *req) {
      this->onget(res, req);
    });

    // Deleting a document.
    app.del("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->ondel(res, req);
    });
  }
//-------------------------------------------------------------------------//
  template auto routes::register_routes<uWS::TemplatedApp<false>>(uWS::TemplatedApp<false>& app) -> void;
  template auto routes::register_routes<uWS::TemplatedApp<true>>(uWS::TemplatedApp<true>& app) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::http
