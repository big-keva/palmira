/*!==========================================================================
* \file
* - Program:       docapi-http
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
#include <string>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include <structo/contents.hpp>
//-------------------------------------------------------------------------//
#include <common/module-utils.h>
//-------------------------------------------------------------------------//
#include "../../../include/server.h"
//-------------------------------------------------------------------------//
#include "async-executer.h"
//-------------------------------------------------------------------------//
#include "../http/errors.h"
#include "../http/docreq.h"
#include "../http/docres.h"
//-------------------------------------------------------------------------//
#include "../json/elastic/simd-json-index-parser.h"
#include "../json/elastic/simd-json-search-parser.h"
#include "../json/elastic/simd-json-mget-parser.h"
//-------------------------------------------------------------------------//
#include "../http/docres.h"
//-------------------------------------------------------------------------//
namespace docapi {
//-------------------------------------------------------------------------//
  struct async_response_state final {
    std::atomic_bool aborted{false};
  };
//-------------------------------------------------------------------------//
  //!< Keeps a max size of body.
  constexpr std::uint64_t max_body_size = 5*1024*1024;
  //!< Keeps a request timeout (in milliseconds).
  constexpr std::uint32_t request_timeout = 30000;
//-------------------------------------------------------------------------//
  class HttpServer : public palmira::IServer {
    using storage_t = mtc::api<structo::IStorage>;

    // Keeps executer context.
    struct executer_context final {
      //!< Keeps a type of executer.
      const executer_types type;
    };

    //!< Keeps a server config.
    const mtc::zmap settings;

    //!< Keeps a search service.
    service_t service;

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
     * @param settings [in] - A server configuration.
     */
    explicit HttpServer(mtc::api<palmira::IService> service, mtc::zmap settings);

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

    template<bool SSL>
    auto register_routes(uWS::TemplatedApp<SSL> &app) -> void;

    template<bool SSL, typename Response, typename Request>
    auto onpost(Response *res, Request *req) -> void;

    template<bool SSL, typename Response, typename Request>
    auto onput(Response *res, Request *req) -> void;

    template <bool SSL, typename Response, typename Request>
    auto onget(Response *res, Request *req) -> void;

/*
    template<typename Response, typename Fn>
    auto onsubmit(Response *res, const std::shared_ptr<async_response_state>& state, Fn &&fn) -> void;
*/

  private:
    auto onloop() -> void;
    auto onlisten(us_listen_socket_t *token) -> void;
  };
//-------------------------------------------------------------------------//
  template <bool SSL>
  auto HttpServer::register_routes(uWS::TemplatedApp<SSL> &app) -> void {
    // Adds a new document and automatically generates a unique ID.
    app.post("/:index/_doc", [this](auto *res, auto *req) {
      this->onpost<SSL>(res, req);
    });
    // Adds a new document with a specified ID or updates an existing document with the same ID.
    app.put("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->onput<SSL>(res, req);
    });

    // Adding a new document with a specified ID only if a document with that ID does not already exist.
    // If the document exists, the operation fails.
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

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onpost(Response *res, Request *req) -> void {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);
    auto body = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? palmira::modules::make_uid() : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());
    const auto body_size = this->settings.get_int64("max_body_size", max_body_size);
    const auto request_started = std::chrono::steady_clock::now();

    std::fprintf(stdout, "TRACE Received POST request: index=%s, id=%s\n", index.c_str(), id.c_str());

    // Reserving resources.
    body->reserve(body_size);

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    res->onData([this, resp_ctx, state, index, id, params, body, body_size, started = request_started.time_since_epoch().count()](std::string_view chunk, bool last) mutable {
      const auto timeout = this->settings.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire)) {
        return;
      }
      if (body->size() + chunk.size() > body_size) {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      body->append(chunk.data(), chunk.size());

      if (not last) {
        return;
      }

      // Forwarding a request to search engine.
      this->executer.enqueue(executer_types::insert, [loop = this->loop, service = this->service, resp_ctx, index, id, body = std::move(*body), params, timeout, &state, started](const executer_context &ctx) mutable -> void {
        try {
          // Parsing insert request.
          auto args = json::elastic::parse_index_request(std::string_view(body), mtc::zmap{
            {"_index",   index},
            {"_id",      id},
            {"_params",  params},
            {"_started", started}
          });

          // Sending a document to search engine.
          service->Insert(args, [&ctx, &loop, resp_ctx, &state](const mtc::zmap &resp) {
            auto reply = http::service_response{
              .status = 200,
              .content_type = "application/json; charset=utf-8",
              .body = ""
            };

            // Making a response.
            reply.body = http::elastic::make_index_response(resp);

            loop->defer([resp_ctx, state, reply = std::move(reply)]() mutable {
              if (state->aborted.load(std::memory_order_acquire)) {
                return;
              }

              // Replying a response to client.
              http::send_json_response(resp_ctx.get(), reply);
            });
          });
        } catch (const std::exception &exc) {
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        } catch (...) {
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onput(Response *res, Request *req) -> void {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);
    auto body = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = http::parse_query(req->getQuery());
    const auto body_size = this->settings.get_int64("max_body_size", max_body_size);
    const auto request_started = std::chrono::steady_clock::now();

    std::fprintf(stdout, "TRACE Received PUT request: index=%s, id=%s\n", index.c_str(), id.c_str());

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (id.empty()) {// This ID is required
      state->aborted.store(true, std::memory_order_release);

      // Replying error message.
      http::send_error_response(resp_ctx.get(), http::status_codes::BAD_REQUEST, "pad_request", "document id is empty");
    }

    // Reserving resources.
    body->reserve(body_size);

    res->onData([this, resp_ctx, state, index, id, params, started = request_started.time_since_epoch().count(), body, body_size](std::string_view chunk, bool last) mutable {
      const auto timeout = this->settings.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire)) {
        return;
      }

      if (body->size() + chunk.size() > body_size) {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "payload_too_large", "request body is too large");
        return;
      }
      body->append(chunk.data(), chunk.size());

      if (not last) {
        return;
      }

      // Forwarding a request to search engine.
      this->executer.enqueue(executer_types::insert, [loop = this->loop, service = this->service, resp_ctx, index, id, body = std::move(*body), params, timeout, state, started](const executer_context &ctx) mutable -> void {
        auto reply = http::service_response{
          .status = static_cast<int>(http::status_codes::OK),
          .content_type = "application/json; charset=utf-8",
          .body = ""
        };

        try {
          // Parsing insert request.
          auto args = json::elastic::parse_index_request(std::string_view(body), mtc::zmap{
            {"_index",      index},
            {"_id", id},
            {"_params",     params},
            {"_started",    started}
          });

          // Sending a document to search engine.
          auto resp = service->Insert(args, [&ctx, &loop, resp_ctx, &state](const mtc::zmap &resp) {
            auto reply = http::service_response{
              .status = 200,
              .content_type = "application/json; charset=utf-8",
              .body = ""
            };

            // Making a response.
            reply.body = http::elastic::make_index_response(resp);

            loop->defer([resp_ctx, state, reply = std::move(reply)]() mutable {
              if (state->aborted.load(std::memory_order_acquire)) {
                return;
              }

              // Replying a response to client.
              http::send_json_response(resp_ctx.get(), reply);
            });
          });
        } catch (const std::exception &exc) {
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        } catch (...) {
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", "unknown");
        }
      });
    });
  }

  template<bool SSL, typename Response, typename Request>
  auto HttpServer::onget(Response *res, Request *req) -> void {
    auto state = std::make_shared<async_response_state>();
    auto resp_ctx = std::make_shared<http::response_context<SSL>>(res);

    const auto params = http::parse_query(req->getQuery());
    const auto routing = http::get_query_param(params, "routing");
    const auto index = std::string(req->getParameter(0));
    const auto id = std::string(req->getParameter(1));
    const auto body_size = this->settings.get_int64("max_body_size", max_body_size);
    const auto request_started = std::chrono::steady_clock::now();

    std::fprintf(stdout, "TRACE Received GET request: index=%s, id=%s\n", index.c_str(), id.c_str());

    res->onAborted([resp_ctx]() {
      resp_ctx->aborted.store(true, std::memory_order_release);
    });
    auto body = std::make_shared<std::string>();

    // Reserving resources.
    body->reserve(body_size);

    // Reading payload.
    res->onData([this, resp_ctx, index, id, params, routing, state, body, body_size, request_started](std::string_view chunk, bool last) mutable {
      const auto timeout = this->settings.get_int32("request_timeout", request_timeout);

      if (state->aborted.load(std::memory_order_acquire)) {
        return;
      }
      if (body->size() + chunk.size() > body_size) {
        state->aborted.store(true, std::memory_order_release);

        // Replying error message.
        http::send_error_response(resp_ctx.get(), http::status_codes::PAYLOAD_TOO_LARGE, "payload_too_large", "request body is too large");
        return;
      }
      body->append(chunk.data(), chunk.size());

      if (not last) {
        return;
      }

      //<!!!> uWebSockets response lives in original event-loop inside.
      this->executer.enqueue(executer_types::search, [service = this->service, resp_ctx, index, id, params, routing, body = std::move(*body), started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()](const executer_context &ctx) mutable {
        try {
          if (service == nullptr) {
            throw (std::invalid_argument("Search engine not created"));
          }

          palmira::SearchArgs args;
          // Parsing a search request.
          const auto req = json::elastic::parse_search_request(body, {index});

          // Setting an order of searching.
          args.order = mtc::zmap{
            {"first", req.from},
            {"count", req.size}
          };

          args.query = mtc::zmap{
           {"id", id}
          };

          // Forwarding a search request into search engine.
          service->Search(args, [resp_ctx, index, ctx, state, loop](const mtc::zmap &resp) {
            auto reply = http::service_response{
              .status = 200,
              .content_type = "application/json; charset=utf-8",
              .body = ""
            };

            // Making a response.
            reply.body = http::elastic::make_search_response(mtc::zmap{
              {"resp",   resp},
              {"_index", index}
            });

            loop->defer([resp_ctx, state, reply = std::move(reply)]() mutable {
              if (state->aborted.load(std::memory_order_acquire)) {
                return;
              }

              // Replying a response to client.
              http::send_json_response(resp_ctx.get(), reply);
            });
          });
        } catch (const std::exception &exc) {
          std::fprintf(stderr, "ERROR Proceeding request failed: %s\n", exc.what());
          // Replying to error response.
          http::send_error_response(resp_ctx.get(), http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        } catch (...) {
          std::fprintf(stderr, "ERROR Proceeding request failed: unknown\n");
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
} // namespace docapi
//-------------------------------------------------------------------------//
#endif // __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__

