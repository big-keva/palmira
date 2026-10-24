#include "routes.h"
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "../../common/thread-pool.h"
#include "../../common/utils.h"
//-------------------------------------------------------------------------//
#include "../../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../../http/http-req.h"
#include "../../http/http-resp.h"
//-------------------------------------------------------------------------//
#include "../../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  auto get_thread_pool() -> elastic::thread_pool *;
  auto get_ws_loop() -> uWS::Loop *;
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
namespace elastic::searchapi::http
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
    template<typename Response, typename OnReply>
    auto reply_resp(uWS::Loop *ctx,
                    Response *res,
                    const std::shared_ptr<async_response_state> &state,
                    mtc::zmap resp,
                    OnReply &&onreply) -> void
    {
      if (ctx != nullptr)
      {
        ctx->defer([res, state, resp = std::move(resp), onreply = std::forward<OnReply>(onreply)]() mutable {
          if (state->aborted.load(std::memory_order_acquire))
          {
            return;
          }

          LOG_T_C("Replying a response: %s", mtc::to_string(resp).c_str());
          // Replying a response to client.
          std::invoke(onreply, res, std::as_const(resp));
        });
      }
    }
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
    auto payload = std::make_shared<std::string>();

    const auto _index = std::string(req->getParameter(0));
    const auto _id = req->getParameter(1).empty() ? elastic::make_uid() : std::string(req->getParameter(1));
    const auto params = elastic::http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_D_C("Received POST request: index=%s, id=%s", _index.c_str(), _id.c_str());
    // Reserving resources.
    payload->reserve(body_size);

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (g_thread_pool == nullptr)
    {
      g_thread_pool = get_thread_pool();
    }
    res->onData([this, res, state, _index, _id, params, payload, body_size, started = request_started.time_since_epoch().count()](std::string_view chunk, bool last) mutable {
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
      g_thread_pool->enqueue([service = this->service, res, _index, _id, body = std::move(*payload), params, timeout, &state, started]() mutable -> void {
          try
          {
            LOG_T_C("Received POST request: index=%s, id=%s, payload=%s", _index.c_str(), _id.c_str(), body.c_str());
/*<TODO>
            // Replying a response.
            reply_resp(g_ws_loop, res, state, std::move(resp), [_index, _id](Response *res, const mtc::zmap &resp) {
              // Replying a response to client.
              http::send_index_json_response(res, resp, _index, _id);
            });
*/
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

    const auto _index = std::string(req->getParameter(0));
    const auto _id = req->getParameter(1).empty() ? std::string{} : std::string(req->getParameter(1));
    const auto params = elastic::http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (_id.empty())
    { // This ID is required
      state->aborted.store(true, std::memory_order_release);

      // Replying error message.
      elastic::http::send_error_response(res, elastic::http::status_codes::BAD_REQUEST, "pad_request", "document id is empty");
    }

    // Reserving resources.
    payload->reserve(body_size);

    res->onData([this, res, state, _index, _id, params, started = request_started.time_since_epoch().count(), payload, body_size](std::string_view chunk, bool last) mutable {
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
      g_thread_pool->enqueue([service = this->service, res, _index, _id, payload, params, timeout, state, started]() mutable -> void {
        try
        {
          LOG_T_C("Received PUT request: index=%s, id=%s, payload=%s", _index.c_str(), _id.c_str(), payload->c_str());

/*<TODO>
          // Replying a response.
          reply_resp(g_ws_loop, res, state, std::move(resp), [_index, _id](Response *res, const mtc::zmap &resp) {
          });
*/
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
  //-------------------------------------------------------------------------//
  template<typename app_t>
  auto routes::register_routes(app_t &app) -> void
  {
    // Saving uWS event loop.
    g_ws_loop = get_ws_loop();
    assert(g_ws_loop != nullptr && "Invalid pointer on uWS event loop");

    g_thread_pool = get_thread_pool();
    assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");

    app.post("/_search", [this](auto *res, auto *req) {
      this->onpost(res, req);
    });

    app.post("/:index/_search", [this](auto *res, auto *req) {
      this->onpost(res, req);
    });
  }
//-------------------------------------------------------------------------//
  template auto routes::register_routes<uWS::TemplatedApp<false>>(uWS::TemplatedApp<false> &app) -> void;
  template auto routes::register_routes<uWS::TemplatedApp<true>>(uWS::TemplatedApp<true> &app) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::http
