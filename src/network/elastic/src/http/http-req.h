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
#ifndef __HTTP_REQ_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __HTTP_REQ_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
#include "errors.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  using query_params_t = std::unordered_map<std::string, std::string>;
//-------------------------------------------------------------------------//
  /**
   * Parses query string into a map of key-value pairs.
   * @param query [in] - A query string to parse.
   * @return A map of key-value pairs.
   */
  mtc::zmap parse_query(std::string_view query);

  /**
   * Gets a value of parameter by its name.
   * @param params [in] - A list of request parameters.
   * @param name [in] - Parameter name.
   * @param default_value [in] - Default value to return if parameter is not found.
   * @return Parameter value or default value if parameter is not found.
   */
  auto get_query_param(const mtc::zmap &params, std::string_view name, std::optional<std::string> default_value = std::nullopt) -> std::optional<std::string>;
//-------------------------------------------------------------------------//
} // namespace elastic::http
//-------------------------------------------------------------------------//
#endif // __HTTP_REQ_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
