/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-bool-query-parser.h
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
#ifndef __SIMD_JSON_BOOL_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_BOOL_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
#include "simd-json-query-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic::search {
//-------------------------------------------------------------------------//
  struct query_node;
  using query_node_t = std::unique_ptr<query_node>;
//-------------------------------------------------------------------------//
  struct bool_query {
    std::vector<query_node_t> must;
    std::vector<query_node_t> should;
    std::vector<query_node_t> filter;
    std::vector<query_node_t> must_not;

    std::uint64_t minimum_should_match = 0;
    double boost = 1.0;
  };
//-------------------------------------------------------------------------//
  using bool_query_t = std::shared_ptr<bool_query>;
//-------------------------------------------------------------------------//
  /**
   * Parsing a pool value.
   * @param value [in] - A value.
   * @param output [in, out] - A query.
   * @param context [in] - A context.
   */
  auto parse_bool_clause(simdjson::ondemand::value value, std::vector<query_node_t> &output, std::string_view context) -> void;

  /**
   * Parses a bool query.
   * @param bool_object [in] - A bool object.
   * @return A bool query.
   */
  auto parse_bool_query(simdjson::ondemand::object bool_object) -> bool_query_t;
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_BOOL_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
