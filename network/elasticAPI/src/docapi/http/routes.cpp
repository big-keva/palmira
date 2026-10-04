#include "routes.h"
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "docapi-req.h"
#include "docapi-resp.h"
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
#include "../json/simd-json-index-parser.h"
#include "../json/simd-json-update-parser.h"
#include "../json/simd-json-get-parser.h"
#include "../json/simd-json-mget-parser.h"
#include "../json/simd-json-bulk-parser.h"
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
    auto payload = std::make_shared<std::string>();

    const auto index = std::string(req->getParameter(0));
    const auto id = req->getParameter(1).empty() ? elastic::make_uid() : std::string(req->getParameter(1));
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
              {"_index",   index},
              {"_id",      id},
              {"_params",  params},
              {"_started", started}
            });

            // Sending a document to search engine.
            service->Insert(args, [res, state, index, id](const mtc::zmap &resp) {
              LOG_T_C("Received a response: %s", mtc::to_string(resp).c_str());

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
  auto routes::onbulk(Response *res, Request *req) -> void
  {
    auto state = std::make_shared<async_response_state>();
    auto payload = std::make_shared<std::string>();

    const auto _index = std::string(req->getParameter(0));
    const auto query_params = elastic::http::parse_query(req->getQuery());
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

/*
    { "index" : { "_index" : "test", "_id" : "1" } }
    { "field1" : "value1" }
    { "delete" : { "_index" : "test", "_id" : "2" } }
    { "create" : { "_index" : "test", "_id" : "3" } }
    { "field1" : "value3" }
    { "update" : {"_id" : "1", "_index" : "test"} }
    { "doc" : {"field2" : "value2"} }
*/
    // Reserving resources.
    payload->reserve(body_size);

    LOG_D_C("Received BULK request");
    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    if (g_thread_pool == nullptr)
    {
      g_thread_pool = get_thread_pool();
    }

    // Reading a payload of request.
    res->onData([this, res, state, query_params, payload, _index, body_size, started = request_started.time_since_epoch().count()](std::string_view chunk, bool last) mutable {
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
      g_thread_pool->enqueue([service = this->service, res, body = std::move(*payload), query_params, default_index = _index, timeout, &state, started]() mutable -> void {
          try
          {
            mtc::zmap bulk_resp;
            uint64_t requests_count = 0;
            LOG_D_C("Received BULK request: payload=%s", body.c_str());

            // Parsing bulk request.
            json::parse_bulk_request(std::string_view(body), [&](json::request_types type, const json::bulk_params_t &command_params, std::string_view body) {
              auto get_cmd_param_value = [](const json::bulk_params_t &params, std::string_view name, std::string_view default_value = "") -> std::string_view {
                auto found = params.find(name.data());
                if (found != std::end(params))
                {
                  return not found->second.empty() ? found->second : default_value;
                }
                return default_value;
              };
              LOG_T_C("Received BULK request: %s", body.data());
              auto set_http_code = [](json::request_types type, mtc::zmap &resp) {
                auto status = resp.get_zmap("status");
                if (status != nullptr)
                {
                  const auto status_code = status->get_int32("code", -1);
                  if (type == json::request_types::create || type == json::request_types::index)
                  {
                    status->set_word16("http_code", static_cast<mtc::word16_t>(status_code == 0
                      ? elastic::http::status_codes::CREATED
                      : elastic::http::status_codes::BAD_REQUEST));
                  }
                  else if (type == json::request_types::remove || type == json::request_types::update)
                  {
                    status->set_word16("http_code", static_cast<mtc::word16_t>(status_code == 0
                      ? elastic::http::status_codes::OK
                      : elastic::http::status_codes::NOT_FOUND));
                  }
                }
              };

              mtc::zmap resp = {};
              const auto _index = default_index.empty() ? std::string(get_cmd_param_value(command_params, "_index")) : default_index;
              const auto _id = std::string(get_cmd_param_value(command_params, "_id", make_uid()));
              const auto _version = std::string(get_cmd_param_value(command_params, "_version", "0"));

              if (_index.empty()) { LOG_E_C("Invalid request command [%s]: index is empty", to_string(type).data()); return; }
              if (_id.empty()) { LOG_E_C("Invalid request command [%s]: index is empty", to_string(type).data()); return; }

              auto params = mtc::zmap{
                {"_index",   _index},
                {"_id",      _id},
                {"_params",  query_params},
                {"_started", started}
              };

              switch (type)
              {
                case json::request_types::create:
                case json::request_types::index: {
                  // Making insert request to the engine.
                  const auto args = json::parse_index_request(body, params);

                  // Sending a document to search engine.
                  resp = service->Insert(args)->Wait(timeout);
                  break;
                }
                case json::request_types::update: {
                  // Making update request to the engine.
                  auto args = json::parse_update_request(body, params);

                  // Sending a document to search engine.
                  resp = service->Insert(args)->Wait(timeout);
                  break;
                }
                case json::request_types::remove: {
                  palmira::RemoveArgs args; //<??? - sigfault> (_id, std::atoi(_version.c_str()));

                  args.objectId = _id;
                  args.uVersion = std::atoi(_version.c_str());

                  // Forwarding a remove request into the engine.
                  resp = service->Remove(args)->Wait(timeout);
                  break;
                }
                default:
                  LOG_W_C("Unsupported request type: %s", to_string(type).data());
              }

              LOG_T_C("Received a response from the engine: %s", mtc::to_string(resp).c_str());
              if (resp.empty())
              {
                LOG_E_C("Received index/create request failed: timeout");
                return;
              }
              auto *array_zmap = bulk_resp.get_array_zmap(to_string(type).data());
              if (array_zmap == nullptr)
              {
                array_zmap = bulk_resp.set_array_zmap(to_string(type).data());
              }

              if (array_zmap != nullptr)
              {
                // Incrementing a count of requests.
                ++requests_count;

                // Setting HTTP status code.
                set_http_code(type, resp);

                // Adding a response.
                array_zmap->push_back(std::move(resp));
              }
            });

            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, bulk_resp, requests_count, state]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }

                // Adding a count of requests.
                bulk_resp.set_int64("requests_count", static_cast<std::int64_t>(requests_count));

                LOG_T_C("Replying BULK response: %s", mtc::to_string(bulk_resp).c_str());
                // Replying a response to client.
                http::send_bulk_json_response(res, bulk_resp);
              });
            }
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
  auto routes::onmget(Response *res, Request *req) -> void
  {
    auto payload = std::make_shared<std::string>();
    auto state = std::make_shared<async_response_state>();

    const auto params = elastic::http::parse_query(req->getQuery());
    const auto _index = std::string(req->getParameter(0));
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_T_C("Received MGET request: _index=%s", _index.c_str());
    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    // Reserving resources.
    payload->reserve(body_size);

    // Reading payload.
    res->onData([this, res, _index, params, state, payload, body_size, request_started](std::string_view chunk, bool last) mutable {
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
      g_thread_pool->enqueue([service = this->service, res, _index, params, payload, started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()]() mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          // Parsing a search request.
          palmira::SearchArgs args = elastic::docapi::json::parse_mget_request(*payload, _index);

          // Forwarding a search request into search engine.
          service->Search(args, [res, _index, state](const mtc::zmap &resp) {
#ifdef __DEBUG__
            mtc::json::Print(stdout, resp, mtc::json::print::decorated());
#else
            LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
#endif // __DEBUG__
            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, resp, state, _index]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }

                LOG_T_C("Replying MGET response: =%s", mtc::to_string(resp).c_str());
                // Replying a response to client.
                http::send_mget_json_response(res, resp);
              });
            }
          });
        }
        catch (const std::exception &exc)
        {
          LOG_E_C("Proceeding MGET request failed: %s", exc.what());
          // Replying to error response.
          elastic::http::send_error_response(res, elastic::http::status_codes::INTERNAL_SERVER_ERROR, "internal_server_error", exc.what());
        }
        catch (...)
        {
          LOG_E_C("Proceeding MGET request failed: unknown");
          // Replying to error response.
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

    const auto _index = std::string(req->getParameter(0));
    const auto _id = std::string(req->getParameter(1));
    const auto params = elastic::http::parse_query(req->getQuery());
    const auto routing = elastic::http::get_query_param(params, "routing");
    const auto body_size = static_cast<size_t>(this->config.get_int64("max_body_size", max_body_size));
    const auto request_started = std::chrono::steady_clock::now();

    LOG_T_C("Received GET request: index=%s, id=%s", _index.c_str(), _id.c_str());
    res->onAborted([state]() {
      state->aborted.store(true, std::memory_order_release);
    });

    // Reserving resources.
    payload->reserve(body_size);

    // Reading payload.
    res->onData([this, res, _index, _id, params, routing, state, payload, body_size, request_started](std::string_view chunk, bool last) mutable {
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
      g_thread_pool->enqueue([service = this->service, res, _index, _id, params, routing, payload, started = request_started.time_since_epoch().count(), state, loop = uWS::Loop::get()]() mutable {
        try
        {
          if (service == nullptr)
          {
            throw(std::invalid_argument("Search engine not created"));
          }

          // Parsing a search request.
          const auto args = json::parse_get_request(_index, _id, params);

          // Forwarding a search request into search engine.
          service->Search(args, [res, _index, _id, state](const mtc::zmap &resp) {
            LOG_T_C("Received GET response: %s", mtc::to_string(resp).c_str());
            if (g_ws_loop != nullptr)
            {
              g_ws_loop->defer([res, resp, state, _index, _id]() mutable {
                if (state->aborted.load(std::memory_order_acquire))
                {
                  return;
                }

                LOG_T_C("Replying a response: %s", mtc::to_string(resp).c_str());
                // Replying a response to client.
                http::send_get_json_response(res, resp, _index, _id);
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
            throw (std::invalid_argument("Search engine not created"));
          }

          palmira::RemoveArgs args;

          args.objectId = id;
          // Forwarding a remove request into the engine.
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
    app.post("/:index/_doc/:id", [this](auto *res, auto *req) {
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

    // Packaging requests handling.
    app.post("/:index/_bulk", [this](auto *res, auto *req) {
      this->onbulk(res, req);
    });
    app.post("/:index/_bulk", [this](auto *res, auto *req) {
      this->onbulk(res, req);
    });
    app.put("/_bulk", [this](auto *res, auto *req) {
      this->onbulk(res, req);
    });
    app.put("/:index/_bulk", [this](auto *res, auto *req) {
      this->onbulk(res, req);
    });

    // Reading packaging documents handling.
    app.post("/:index/_mget", [this](auto *res, auto *req) {
      this->onmget(res, req);
    });
    app.post("/:index/_mget", [this](auto *res, auto *req) {
      this->onmget(res, req);
    });
    app.put("/_mget", [this](auto *res, auto *req) {
      this->onmget(res, req);
    });
    app.put("/:index/_mget", [this](auto *res, auto *req) {
      this->onmget(res, req);
    });

    // Getting a source info by id.
    app.get("/:index/_source/:id", [this](auto *res, auto *req) {
      this->onget(res, req);
    });

    // Deleting a document by id.
    app.del("/:index/_doc/:id", [this](auto *res, auto *req) {
      this->ondel(res, req);
    });
  }
//-------------------------------------------------------------------------//
  template auto routes::register_routes<uWS::TemplatedApp<false>>(uWS::TemplatedApp<false>& app) -> void;
  template auto routes::register_routes<uWS::TemplatedApp<true>>(uWS::TemplatedApp<true>& app) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::http
