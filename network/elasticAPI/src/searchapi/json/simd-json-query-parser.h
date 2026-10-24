#pragma once
//-------------------------------------------------------------------------//
#include <variant>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../../json/simd-json-common.h"
//-------------------------------------------------------------------------//
#include "simd-json-bool-query-parser.h"
#include "simd-json-match-query-parser.h"
#include "simd-json-range-query-parser.h"
#include "simd-json-term-query-parser.h"
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  struct bool_query;
//-------------------------------------------------------------------------//
  //!< Supported queries.
  struct query_node
  {
    //!< Keeps a type of query.
    elastic::json::query_kinds kind;

    //!< Keeps a query.
    std::variant<match_query,
      term_query,
      terms_query,
      range_query,
      std::shared_ptr<bool_query>
      // exists_query,
      // ids_query
    > data;
  };
//-------------------------------------------------------------------------//
  using query_node_t = std::unique_ptr<query_node>;
//-------------------------------------------------------------------------//
  /**
   * Parses a range query.
   * @param query_object [in] - A query object.
   * @return A query.
   */
  auto parse_query_object(simdjson::ondemand::object query_object) -> query_node_t;
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
