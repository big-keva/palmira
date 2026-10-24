#pragma once
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
#include "simd-json-query-parser.h"
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  struct query_node;
//-------------------------------------------------------------------------//
  using query_node_t = std::unique_ptr<query_node>;
//-------------------------------------------------------------------------//
  struct bool_query
  {
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
   * @param context [in] - A context.
   * @param output [in, out] - A query.
   */
  auto parse_bool_clause(simdjson::ondemand::value value,
                         std::string_view context,
                         std::vector<query_node_t> &output) -> void;

  /**
   * Parses a bool query.
   * @param bool_object [in] - A bool object.
   * @return A bool query.
   */
  auto parse_bool_query(simdjson::ondemand::object bool_object) -> bool_query_t;
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
