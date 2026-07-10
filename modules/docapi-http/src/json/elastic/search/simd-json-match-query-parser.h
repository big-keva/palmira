/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-match-query-parser.h
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
#ifndef __SIMD_JSON_MATCH_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_MATCH_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
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
struct match_query
{
  //!< Keeps a name of field, which executes a match.
  std::string field;

  //!< Keeps a query.
  std::string query;

  //!< operator: "or" / "and".
  std::string operator_value;

  //!< fuzziness: например "AUTO".
  std::string fuzziness;

  //!< analyzer.
  std::string analyzer;

  //!< Keeps a minimum_should_match.
  std::string minimum_should_match;

  //!< Keeps zero terms query.
  std::string zero_terms_query; // "none" / "all"

  bool lenient = false;
  bool auto_generate_synonyms_phrase_query = true;

  //!< Keeps a boost.
  double boost = 1.0;
};
//-------------------------------------------------------------------------//
/**
 * Parses a match query.
 * @param match_object  [in] - A match object.
 * @return A match query.
 * @note
 *
 * 1. simple:
 *    { "match": { "name": "brave" } }
 *
 * 2. full:
 *    {
 *      "match": {
 *        "name": {
 *          "query": "brave",
 *          "operator": "and",
 *          "analyzer": "standard",
 *          "fuzziness": "AUTO",
 *          "minimum_should_match": "75%",
 *          "zero_terms_query": "none",
 *          "lenient": false,
 *          "auto_generate_synonyms_phrase_query": true,
 *          "boost": 1.5
 *        }
 *      }
 *    }
 */
auto parse_match_query(simdjson::ondemand::object match_object) -> match_query;
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_MATCH_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
