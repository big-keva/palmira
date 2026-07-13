#include "simd-json-range-query-parser.h"
//-------------------------------------------------------------------------//
#include "../../json/simd-json-errors.h"
#include "../../json/simd-json-common.h"
//-------------------------------------------------------------------------//
namespace elastic::json::searchapi
{
//-------------------------------------------------------------------------//
  auto parse_range_query(simdjson::ondemand::object range_object) -> range_query
  {
    range_query result;

    for (auto field_result : range_object)
    {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read range field");

      result.field = copy_string(read_key(field));

      simdjson::ondemand::object params;
      throw_if_error(field.value().get_object().get(params), "range field value must be object");

      for (auto param_result : params)
      {
        simdjson::ondemand::field param;
        throw_if_error(std::move(param_result).get(param), "failed to read range parameter");

        const auto key = read_key(param);
        simdjson::ondemand::value value = param.value();

        if (key == "gt")
        {
          result.gt = read_scalar_as_string(value, "range.gt");
        }
        else if (key == "gte")
        {
          result.gte = read_scalar_as_string(value, "range.gte");
        }
        else if (key == "lt")
        {
          result.lt = read_scalar_as_string(value, "range.lt");
        }
        else if (key == "lte")
        {
          result.lte = read_scalar_as_string(value, "range.lte");
        }
        else if (key == "format")
        {
          result.format = read_string_value(value, "range.format must be string");
        }
        else if (key == "time_zone")
        {
          result.time_zone =
              read_string_value(value, "range.time_zone must be string");
        }
        else if (key == "relation")
        {
          result.relation =
              read_string_value(value, "range.relation must be string");
        }
        else if (key == "boost")
        {
          result.boost = read_double_value(value, "range.boost must be double");
        }
      }

      if (result.field.empty())
      {
        throw (parse_error("range query field is empty"));
      }

      if (not result.gt && not result.gte && not result.lt && not result.lte)
      {
        throw (parse_error("range query requires at least one of gt/gte/lt/lte"));
      }

      return result;
    }

    throw (parse_error("range query must contain field"));
  }
//-------------------------------------------------------------------------//
} // namespace elastic::json::searchapi
