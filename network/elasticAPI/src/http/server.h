#pragma once
//-------------------------------------------------------------------------//
#include <functional>
//-------------------------------------------------------------------------//
#include "method-type.h"
#include "server.hpp"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  struct ElasticServer : public palmira::IServer
  {
    /**
     * Adds a new HTTP route.
     * @param type [in] - A type of method.
     * @param path [in] - A HTTP path.
     * @param onadd [in] - A callback method.
     */
    virtual auto add_route(http::method_types type, std::string_view path, std::function<void()> onadd) -> void = 0;

    /**
     * Destructor
     */
    virtual ~ElasticServer() = default;

    implement_lifetime_control
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http
