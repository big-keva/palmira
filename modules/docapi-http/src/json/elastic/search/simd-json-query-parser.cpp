#include "simd-json-bool-query-parser.h"
//-------------------------------------------------------------------------//
#include "../../simd-json-errors.h"
#include "../../simd-json-common.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic::search {
//-------------------------------------------------------------------------//
  auto parse_query_object(simdjson::ondemand::object query_object) -> query_node_t {
    auto query = std::make_unique<query_node>();

    query->kind = query_kinds::none;
    query->data = {};

    for (auto query_field_result : query_object) {
      simdjson::ondemand::field query_field;
      throw_if_error(std::move(query_field_result).get(query_field), "failed to read query field");

      const auto query_key = read_key(query_field);
      simdjson::ondemand::value query_value = query_field.value();

      if (query_key == "match") {
        simdjson::ondemand::object match_object;
        throw_if_error(query_value.get_object().get(match_object), "match query must be object");

        query->kind = query_kinds::match;
        query->data = parse_match_query(match_object);
      } else if (query_key == "term") {
        simdjson::ondemand::object term_object;
        throw_if_error(query_value.get_object().get(term_object), "term query must be object");

        query->kind = query_kinds::term;
        query->data = parse_term_query(term_object);
      } else if (query_key == "terms") {
        simdjson::ondemand::object terms_object;
        throw_if_error(query_value.get_object().get(terms_object), "terms query must be object");

        query->kind = query_kinds::terms;
        query->data = parse_terms_query(terms_object);
      } else if (query_key == "range") {
        simdjson::ondemand::object range_object;
        throw_if_error(query_value.get_object().get(range_object), "range query must be object");

        query->kind = query_kinds::range;
        query->data = parse_range_query(range_object);
      } else if (query_key == "bool") {
        simdjson::ondemand::object bool_object;
        throw_if_error(query_value.get_object().get(bool_object), "bool query must be object");

        query->kind = docapi::json::query_kinds::bool_query;
        query->data = parse_bool_query(bool_object);
      } else if (query_key == "match_all") {
        query->kind = query_kinds::match_all;
        query->data = {};
      } else if (query_key == "match_none") {
        query->kind = query_kinds::match_none;
        query->data = {};
      } else if (query_key == "as_tree") {
        query->kind = query_kinds::as_tree;
        query->data = {};
      }
    }

    return query;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
