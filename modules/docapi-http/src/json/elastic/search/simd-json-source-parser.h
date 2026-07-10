/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-source-parser.h
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
#ifndef __SIMD_JSON_SOURCE_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_SOURCE_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
namespace docapi::json::elastic::search {
//-------------------------------------------------------------------------//
  struct source_filter {
    //!< Keeps a flag of using filter.
    bool enabled = true;

    //!< Keeps a list of includes.
    std::vector<std::string> includes;

    //!< Keeps a list of excludes.
    std::vector<std::string> excludes;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses a source object.
   * @param object [in] - A source object.
   * @param source [out] - A source.
   */
  auto parse_source_object(simdjson::ondemand::object object, source_filter &source) -> void;

  /**
   * Parses a source value.
   * @param value [in] - A source value.
   * @param source [in, out] - A source.
   */
  auto parse_source_value(simdjson::ondemand::value value, source_filter &source) -> void;
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_SOURCE_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
