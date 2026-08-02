#pragma once
//-------------------------------------------------------------------------//
#include <memory>
//-------------------------------------------------------------------------//
#include <structo/contents.hpp>
//-------------------------------------------------------------------------//
#include <mtc/config.h>
#include <mtc/json.h>
//-------------------------------------------------------------------------//
#include "server.hpp"
//-------------------------------------------------------------------------//
#include "../json/simd-json-index-parser.h"
#include "../json/simd-json-mget-parser.h"
#include "network/elasticAPI/src/common/thread-pool.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::http
{
//-------------------------------------------------------------------------//
  //!< Keeps a max size of body.
  constexpr std::uint64_t max_body_size = 5 * 1024 * 1024;
  //!< Keeps a request timeout (in milliseconds).
  constexpr std::uint32_t request_timeout = 30000;
//-------------------------------------------------------------------------//
  class routes final
  {
    using storage_t = mtc::api<structo::IStorage>;

    //!< Keeps a search service.
    mtc::api<palmira::IService> service;

    //!< Keeps a server config.
    const mtc::config config;

  public:
    /**
     * Constructor.
     * @param service [in] - A search service.
     * @param config [in] - A server configuration.
     */
    explicit routes(palmira::IService *service, const mtc::config &config);

    /**
     * Destructor.
     */
    ~routes() = default;

    template<typename app_t>
    auto register_routes(app_t &app) -> void;

  private:
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
  };
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::http
