/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-term-query-parser.h
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
#ifndef __SIMD_JSON_TERM_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_TERM_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
namespace docapi::json::elastic::search
{
//-------------------------------------------------------------------------//
struct term_query
{
  //!< Keeps a name of field.
  std::string field;

  //!< Keeps a value of field.
  std::string value;

  double boost = 1.0;
};

struct terms_query
{
  //!< Keeps a name of field.
  std::string field;

  //!< Keeps a list of field values.
  std::vector<std::string> values;
};
//-------------------------------------------------------------------------//
/**
 * Parses a term query.
 * @param term_object [in] - A term object.
 * @return A term query.
 */
auto parse_term_query(simdjson::ondemand::object term_object) -> term_query;

/**
 * Parses a terms query.
 * @param terms_object [in] - A terms object.
 * @return A terms query.
 */
auto parse_terms_query(simdjson::ondemand::object terms_object) -> terms_query;
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_TERM_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
