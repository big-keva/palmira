#include "simd-json-bool-query-parser.h"
//-------------------------------------------------------------------------//
#include "../../json/simd-json-common.h"
#include "../../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
#include "simd-json-query-parser.h"
//-------------------------------------------------------------------------//
namespace elastic::json::searchapi
{
//-------------------------------------------------------------------------//
  auto parse_bool_clause(simdjson::ondemand::value value,
                         std::string_view context,
                         std::vector<query_node_t> &output) -> void
  {
    auto array_result = value.get_array();
    if (not array_result.error())
    {
      auto array = array_result.value();
      for (auto item_result : array)
      {
        simdjson::ondemand::value item_value;
        throw_if_error(std::move(item_result).get(item_value), context);

        simdjson::ondemand::object item_object;
        throw_if_error(item_value.get_object().get(item_object), "bool clause item must be object");

        output.emplace_back(parse_query_object(item_object));
      }
      return;
    }

    simdjson::ondemand::object object;
    throw_if_error(value.get_object().get(object), "bool clause must be object or array");

    output.emplace_back(parse_query_object(object));
  }

  auto parse_bool_query(simdjson::ondemand::object bool_object) -> bool_query_t
  {
    auto result = std::make_shared<bool_query>();

    for (auto field_result : bool_object)
    {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read bool field");

      const auto key = read_key(field);
      auto value = field.value();

      if (key == "must")
      {
        parse_bool_clause(value, "failed to read bool.must item", result->must);
      }
      else if (key == "should")
      {
        parse_bool_clause(value, "failed to read bool.should item", result->should);
      }
      else if (key == "filter")
      {
        parse_bool_clause(value, "failed to read bool.filter item", result->filter);
      }
      else if (key == "must_not")
      {
        parse_bool_clause(value, "failed to read bool.must_not item", result->must_not);
      }
      else if (key == "minimum_should_match")
      {
        result->minimum_should_match = read_non_negative_integer(value, "bool.minimum_should_match");
      }
      else if (key == "boost")
      {
        result->boost = read_double_value(value, "bool.boost must be double");
      }
    }

    return result;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::json::searchapi
