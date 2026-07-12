/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          http-resp.h
* - Created:       06/24/2026
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
#ifndef __HTTP_RESP_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __HTTP_RESP_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <atomic>
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
#include "errors.h"
//-------------------------------------------------------------------------//
namespace uWS
{
//-------------------------------------------------------------------------//
  template<bool SSL>
  class HttpResponse;
//-------------------------------------------------------------------------//
}// namespace uWS
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  template<bool SSL>
  struct response_context
  {
    //<!!!> The owner of the pointer is uWebSockets. It must be used ONLY in the uWS loop thread before aborting.
    uWS::HttpResponse<SSL> *response = nullptr;

    //!< Keeps a status of aborted or not.
    std::atomic_bool aborted = false;
  };

  struct service_response
  {
    //!< Keeps a response status.
    status_codes status = status_codes::OK;

    //!< Keeps a response content type.
    std::string content_type = "application/json; charset=utf-8";

    //!< Keeps a response body.
    std::string body;
  };
//-------------------------------------------------------------------------//
  template<bool SSL>
  void send_json_response(response_context<SSL> *ctx, const service_response &response);

  template<bool SSL>
  void send_error_response(response_context<SSL> *ctx, status_codes status, std::string_view error_type, std::string_view reason);
//-------------------------------------------------------------------------//
}// namespace elastic::http
//-------------------------------------------------------------------------//
#endif // __HTTP_RESP_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
