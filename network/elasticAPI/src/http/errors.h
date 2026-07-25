/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          errors.h
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
#ifndef __HTTP_ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __HTTP_ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  //!< Supported HTTP error codes.
  enum class status_codes
  {
    OK = 200,

    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 203,

    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    CONFLICT = 409,
    PAYLOAD_TOO_LARGE = 413,
    UNSUPPORTED_MEDIA_TYPE = 415,
    TOO_MANY_REQUESTS = 429,

    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
  };

  struct error_info final
  {
    int code = 0;
    std::string msg;
  };
//-------------------------------------------------------------------------//
  /**
   * Gets status as a string.
   * @param code [in] - A status code.
   * @return A status as a string.
   */
  auto to_string(status_codes code) -> std::string_view;

  /**
   * Makes a JSON error response.
   * @param code [in] - Error code.
   * @param type [in] - Error type.
   * @param reason [in] - Error reason.
   * @return Error JSON string.
   */
  auto make_error_json(status_codes code, std::string type, std::string reason) -> std::string;
//-------------------------------------------------------------------------//
  auto check_resp_on_error(const mtc::zmap &resp) -> error_info;
//-------------------------------------------------------------------------//
} // namespace elastic::http
//-------------------------------------------------------------------------//
#endif // __HTTP_ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
