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
#include "../json/simd-json-load-document.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
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
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http
