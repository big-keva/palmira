#include "routes.h"
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "http-req.h"
#include "http-resp.h"
//-------------------------------------------------------------------------//
#include "../common/thread-pool.h"
//-------------------------------------------------------------------------//
#include "../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  auto get_thread_pool() -> elastic::thread_pool *;
  auto get_ws_loop() -> uWS::Loop *;
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    // Keeps websocket loop.
    uWS::Loop *g_ws_loop = nullptr;

    // Keeps a global pool of threads.
    thread_pool *g_thread_pool = nullptr;

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
  template<typename app_t>
  auto routes::register_routes(app_t &app) -> void
  {
    // Saving uWS event loop.
    g_ws_loop = get_ws_loop();
    assert(g_ws_loop != nullptr && "Invalid pointer on uWS event loop");

    g_thread_pool = get_thread_pool();
    assert(g_thread_pool != nullptr && "Invalid pointer on thread pool");

    app.get("/", [](auto *resp, auto *req) {
#ifdef __DEBUG__
      http::dump_request(req);
#endif // __DEBUG__

      const auto params = http::parse_query(req->getQuery());
      const auto routing = http::get_query_param(params, "routing");
      auto state = std::make_shared<async_response_state>();

      resp->onAborted([state]() {
        state->aborted.store(true, std::memory_order_release);
      });

      // Replying default route.
      http::send_default_response(resp);
    });

    app.any("/*", [](auto *resp, auto *req) {
#ifdef __DEBUG__
      http::dump_request(req);
#endif // __DEBUG__

      const auto params = http::parse_query(req->getQuery());
      const auto routing = http::get_query_param(params, "routing");
      auto state = std::make_shared<async_response_state>();

      resp->onAborted([state]() {
        state->aborted.store(true, std::memory_order_release);
      });

      http::send_default_response(resp)->writeStatus("404 Not Found")
                                       ->writeHeader("Content-Type", "application/json; charset=utf-8")
                                       ->end(R"({"error":"route not found","status":404})");
    });
  }
//-------------------------------------------------------------------------//
  template auto routes::register_routes<uWS::TemplatedApp<false>>(uWS::TemplatedApp<false> &app) -> void;
  template auto routes::register_routes<uWS::TemplatedApp<true>>(uWS::TemplatedApp<true> &app) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::http
