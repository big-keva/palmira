#pragma once
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
#include <mtc/json.h>
//-------------------------------------------------------------------------//
#include "../elastic-api-server.h"
//-------------------------------------------------------------------------//
#include "../common/thread-pool.h"
//-------------------------------------------------------------------------//
#include "../http/method-type.h"
#include "../http/routes.h"
//-------------------------------------------------------------------------//
#include "../docapi/http/routes.h"
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
  class http_server final : public palmira::IServer
  {
    using storage_t = mtc::api<structo::IStorage>;

    //!< Keeps a server config.
    const mtc::config config;

    //!< Keeps a search service.
    mtc::api<palmira::IService> service;

    //!< Keeps a list of servers.
    std::tuple<
      http::routes,         // default routes
      docapi::http::routes  // Document API routes
    > routes;

    std::thread thread;
    std::atomic_bool started{false};
    std::atomic_bool stopping{false};

    std::mutex mtx;
    std::condition_variable cv;
    bool start_failed = false;

    us_listen_socket_t *listen_socket = nullptr;

    implement_lifetime_control

  public:
    /**
     * Constructor.
     * @param service [in] - A search service.
     * @param config [in] - A server configuration.
     */
    explicit http_server(palmira::IService *service, const mtc::config &config);

    /**
     * Destructor.
     */
    ~http_server();

    // Override methods
  public:
    void Start() override;
    void Stop() override;
    void Wait() override;

  protected:
    template<typename app_t>
    auto register_routes(app_t &app) -> void;

    template<typename Response, typename Request>
    auto onpost(Response *res, Request *req) -> void;

    template<typename Response, typename Request>
    auto onput(Response *res, Request *req) -> void;

    template<typename Response, typename Request>
    auto onupdate(Response *res, Request *req) -> void;

    template<typename Response, typename Request>
    auto onget(Response *res, Request *req) -> void;

    template<typename Response, typename Request>
    auto ondel(Response *res, Request *req) -> void;

  private:
    auto onloop() -> void;
    auto onlisten(us_listen_socket_t *token) -> void;
  };
//-------------------------------------------------------------------------//
  /**
   * Gets a listen port.
   * @return A listen port.
   */
  auto getListenPort() -> std::uint16_t;

  /**
   * Gets a pool of threads.
   * @return A thread pool.
   */
  auto get_thread_pool() -> thread_pool *;

  /**
   * Gets uWS event loop.
   * @return uWS event loop.
   */
  auto get_ws_loop() -> uWS::Loop *;
//-------------------------------------------------------------------------//
} // namespace elastic
