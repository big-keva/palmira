/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-parser.h
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
#ifndef __SIMD_JSON_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>
//-------------------------------------------------------------------------//
#include "../simd-json-common.h"
//-------------------------------------------------------------------------//
#include "search/simd-json-query-parser.h"
#include "search/simd-json-source-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic {
//-------------------------------------------------------------------------//
  struct sort_clause {
    std::string field;
    std::string order = "asc";
  };

  struct field_request {
    std::string field;
    std::string format;
  };

  //!< Keeps a search request.
  struct search_request {
    //!< Keeps a list of indices.
    std::vector<std::string> indices;

    std::uint64_t from = 0; //<!!!> Elasticsearch uses index from 0.
    std::uint64_t size = 10;

    //!< Keeps source filter.
    search::source_filter source;

    std::vector<sort_clause> sort;
    std::vector<std::string> stored_fields;
    std::vector<field_request> docvalue_fields;
    std::vector<std::string> fields;
    std::vector<std::string> search_after;

    std::optional<std::string> timeout;

    bool explain = false;
    bool profile = false;
    bool track_scores = false;
    bool version = false;
    bool seq_no_primary_term = false;

    bool track_total_hits_enabled = true;
    std::optional<std::uint64_t> track_total_hits_limit;

    std::uint64_t terminate_after = 0;
    double min_score = 0.0;
    bool has_min_score = false;

    //!< Keeps a request query.
    std::optional<search::query_node_t> query;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses a search request.
   * @param body
   * @param indices
   * @return
   */
  auto parse_search_request(std::string_view body, std::vector<std::string> indices) -> search_request;
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
