/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          http-server.h
* - Created:       06/23/2026
* - Author:        Vitaly Bulganin
* - Description:
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#ifndef __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include <structo/contents.hpp>
//-------------------------------------------------------------------------//
#include "../server.h"
//-------------------------------------------------------------------------//
#include "async-executer.h"
//-------------------------------------------------------------------------//
#include "../common/utils.h"
//-------------------------------------------------------------------------//
#include "../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../http/errors.h"
#include "../http/http-req.h"
#include "../http/http-resp.h"
//-------------------------------------------------------------------------//
#include "../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
#include "../docapi/docapi-req.h"
#include "../docapi/docapi-resp.h"
//-------------------------------------------------------------------------//
#include <mtc/json.h>

#include "../docapi/json/simd-json-index-parser.h"
#include "../docapi/json/simd-json-mget-parser.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  struct async_response_state final
  {
    std::atomic_bool aborted{false};
  };
//-------------------------------------------------------------------------//
  //!< Keeps a max size of body.
  constexpr std::uint64_t max_body_size = 5 * 1024 * 1024;
  //!< Keeps a request timeout (in milliseconds).
  constexpr std::uint32_t request_timeout = 30000;
//-------------------------------------------------------------------------//
  class HttpServer : public palmira::IServer
  {
    using storage_t = mtc::api<structo::IStorage>;

    // Keeps executer context.
    struct executer_context final
    {
      //!< Keeps a type of executer.
      const executer_types type;
    };

    //!< Keeps a server config.
    const mtc::config config;

    //!< Keeps a search service.
    mtc::api<palmira::IService> service;

    //!< Keeps a pool of workers.
    async_executer<executer_context> executer;

    std::thread thread;
    std::atomic_bool started{false};
    std::atomic_bool stopping{false};

    std::mutex mtx;
    std::condition_variable cv;
    bool start_failed = false;

    uWS::Loop *loop = nullptr;
    us_listen_socket_t *listen_socket = nullptr;

  public:
    /**
     * Constructor.
     * @param service [in] - A search service.
     * @param config [in] - A server configuration.
     */
    explicit HttpServer(palmira::IService *service, const mtc::config &config);

    /**
     * Destructor.
     */
    virtual ~HttpServer();

    // Override methods
  public:
    void Start() override;
    void Stop() override;
    void Wait() override;

  protected:
    implement_lifetime_control

    template <bool SSL>
    auto register_routes(uWS::TemplatedApp<SSL> &app) -> void;

    template <bool SSL, typename Response, typename Request>
    auto onpost(Response *res, Request *req) -> void;

    template <bool SSL, typename Response, typename Request>
    auto onput(Response *res, Request *req) -> void;

    template <bool SSL, typename Response, typename Request>
    auto onget(Response *res, Request *req) -> void;

  private:
    auto onloop() -> void;
    auto onlisten(us_listen_socket_t *token) -> void;
  };
//-------------------------------------------------------------------------//
  template<bool SSL>
  auto HttpServer::register_routes(uWS::TemplatedApp<SSL> &app) -> void
  {
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

    // Searching.
    app.get("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onget<SSL>(res, req);
    });

    app.get("/:index/_source/:id", [this](auto *res, auto *req) {
      this->onget<SSL>(res, req);
    });
  }

  template <bool SSL, typename Response, typename Request>
  auto HttpServer::onpost(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<elastic::http::response_context<SSL>>(res);
    auto body = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_D_C("Received POST request: index=%s, id=%s", index.c_str(), id.c_str());
    // Reserving resources.
    body->reserve(body_size);

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    res->onData([this, resp_ctx, state, index, id, params, body, body_size, started = request_started.time_since_epoch().count()](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (body->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      body->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      // Forwarding a request to search engine.
      this->executer.enqueue(executer_types::insert, [loop = this->loop, service = this->service, resp_ctx, index, id, body = std::move(*body), params, timeout, &state, started](const executer_context &ctx) mutable -> void {
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

  template <bool SSL, typename Response, typename Request>
  auto HttpServer::onput(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);
    auto body = std::make_shared<std::string>();

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
    body->reserve(body_size);

    res->onData([this, resp_ctx, state, index, id, params, started = request_started.time_since_epoch().count(), body, body_size](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }

      if (body->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      body->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      // Forwarding a request to search engine.
      this->executer.enqueue(executer_types::insert, [loop = this->loop, service = this->service, resp_ctx, index, id, body = std::move(*body), params, timeout, state, started](const executer_context &ctx) mutable -> void {
        try
        {
          LOG_D_C("Received PUT request: index=%s, id=%s, payload=%s", index.c_str(), id.c_str(), body.c_str());
          // Parsing insert request.
          auto args = json::docapi::parse_index_request(std::string_view(body), mtc::zmap{
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

  template <bool SSL, typename Response, typename Request>
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
    auto body = std::make_shared<std::string>();

    // Reserving resources.
    body->reserve(body_size);

    // Reading payload.
    res->onData([this, resp_ctx, index, id, params, routing, state, body, body_size, request_started](std::string_view chunk, bool last) mutable {
      const auto timeout = this->config.get_int32("request_timeout", request_timeout);
      if (state->aborted.load(std::memory_order_acquire))
      {
        return;
      }
      if (body->size() + chunk.size() > body_size)
      {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::PAYLOAD_TOO_LARGE, "payload_too_large", "request body is too large");
        return;
      }
      body->append(chunk.data(), chunk.size());

      if (not last)
      {
        return;
      }

      //<!!!> uWebSockets response lives in original event-loop inside.
      this->executer.enqueue(executer_types::search, [service = this->service, resp_ctx, index, id, params, routing, body = std::move(body), started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()](const executer_context &ctx) mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          // Parsing a search request.
          const auto req = not body->empty() ? json::docapi::parse_mget_request(*body, index)
                                                        : json::docapi::parse_mget_request(index, id, mtc::zmap{
                                                           {"_index", index},
                                                           {"_id", id},
                                                           {"_routing", routing.has_value() ? routing.value() : ""},
                                                           {"_source", {}}
                                                         });

          palmira::SearchArgs args;
          // Setting an order of searching.
          args.order = mtc::zmap{
            {"first", /* => */  1},
            {"count", /* => */  10},
            {"quote", /* => */  mtc::zmap{{"mode", "source"}}}
          };

          args.query = mtc::zmap{
            {"get",   /* => */  mtc::array_charstr{ id }}
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
            LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
            auto reply = http::service_response{
              .status = http::status_codes::OK,
              .content_type = "application/json; charset=utf-8",
              .body = ""
            };

          // вот здесь вот я тебе возвращаю:
          // "items": [
          //   {
          //     ...
          //     "quote": [
          //       {
          //         "title": ...
          //     ...
            mtc::json::Print( stdout, resp, mtc::json::print::decorated() );

            // Making a response.
            reply.body = http::docapi::make_search_response(mtc::zmap{
              {"resp", resp},
              {"_index", index},
              {"_id", id}
            });

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
  /**
   * Gets a listen port.
   * @return A listen port.
   */
  auto getListenPort() -> std::uint16_t;
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
#endif // __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
