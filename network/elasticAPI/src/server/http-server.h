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
#include <mtc/json.h>
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

    template<bool SSL>
    auto register_routes(uWS::TemplatedApp<SSL> &app) -> void;

    template<bool SSL, typename Response, typename Request>
    auto onpost(Response *res, Request *req) -> void;

    template<bool SSL, typename Response, typename Request>
    auto onput(Response *res, Request *req) -> void;

    template<bool SSL, typename Response, typename Request>
    auto onget(Response *res, Request *req) -> void;

    template<bool SSL, typename Response, typename Request>
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
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
#endif // __HTTP_SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
