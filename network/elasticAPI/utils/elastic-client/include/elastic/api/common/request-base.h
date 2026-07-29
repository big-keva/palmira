#pragma once
//-------------------------------------------------------------------------//
#include "../http/request.h"
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  struct request_base
  {
    /**
     * Build a new HTTP request.
     * @return A new request.
     */
    virtual http::request build() const = 0;

    /**
     * Destructor.
     */
    virtual ~request_base() = default;
  };
//-------------------------------------------------------------------------//
}; // namespace elastic
