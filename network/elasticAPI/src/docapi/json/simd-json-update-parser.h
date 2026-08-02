/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          simd-json-index-parser.h
* - Created:       07/08/2026
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
#ifndef __SIMD_JSON_INDEX_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_INDEX_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string_view>
#include <functional>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
#include <DeliriX/text-API.hpp>
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  /**
   * Parses a document for updating into index.
   * @param body [in] - A document.
   * @param params [in] - A list of query parameters.
   * @return A document in search engine format.
   */
  auto parse_update_request(std::string_view body, const mtc::zmap &params) -> palmira::UpdateArgs;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_INDEX_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
