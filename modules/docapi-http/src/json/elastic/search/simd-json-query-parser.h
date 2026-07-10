/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-query-parser.h
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
#ifndef __SIMD_JSON_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <variant>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../../simd-json-common.h"
//-------------------------------------------------------------------------//
#include "simd-json-bool-query-parser.h"
#include "simd-json-match-query-parser.h"
#include "simd-json-range-query-parser.h"
#include "simd-json-term-query-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic::search
{
//-------------------------------------------------------------------------//
struct bool_query;
//-------------------------------------------------------------------------//
//!< Supported queries.
struct query_node
{
  //!< Keeps a type of query.
  query_kinds kind;

  //!< Keeps a query.
  std::variant<match_query, term_query, terms_query, range_query,
               std::shared_ptr<bool_query>
               // exists_query,
               // ids_query
               >
      data;
};
//-------------------------------------------------------------------------//
using query_node_t = std::unique_ptr<query_node>;
//-------------------------------------------------------------------------//
/**
 * Parses a range query.
 * @param query_object [in] - A query object.
 * @return A query.
 */
auto parse_query_object(simdjson::ondemand::object query_object)
    -> query_node_t;
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
