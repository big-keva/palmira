#pragma once
//-------------------------------------------------------------------------//
#include "../http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::api
{
//-------------------------------------------------------------------------//
  class response_base
  {
  protected:
    //!< Keeps a response.
    http::response response_;

  public:
    /**
     * Constructor.
     * @param resp [in] - A response.
     */
    explicit response_base(http::response resp);
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api
