#include "../simd-json-term-query-parser.h"
//-------------------------------------------------------------------------//
#include "../../../json/simd-json-common.h"
#include "../../../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  auto parse_term_query(simdjson::ondemand::object term_object) -> term_query
  {
    term_query result;

    for (auto field_result : term_object)
    {
      simdjson::ondemand::field field;
      elastic::json::throw_if_error(std::move(field_result).get(field), "failed to read term field");

      result.field = elastic::json::copy_string(elastic::json::read_key(field));
      simdjson::ondemand::value value = field.value();

      auto object_result = value.get_object();
      if (not object_result.error())
      {
        simdjson::ondemand::object params = object_result.value();

        for (auto param_result : params)
        {
          simdjson::ondemand::field param;
          elastic::json::throw_if_error(std::move(param_result).get(param), "failed to read term parameter");

          const auto key = elastic::json::read_key(param);
          simdjson::ondemand::value param_value = param.value();

          if (key == "value")
          {
            result.value = elastic::json::read_scalar_as_string(param_value, "term.value");
          }
          else if (key == "boost")
          {
            result.boost = elastic::json::read_double_value(param_value, "term.boost must be double");
          }
        }
      }
      else
      {
        result.value = elastic::json::read_scalar_as_string(value, "term field value");
      }

      if (result.field.empty())
      {
        throw (elastic::json::parse_error("term query field is empty"));
      }

      if (result.value.empty())
      {
        throw (elastic::json::parse_error("term query requires value"));
      }

      return result;
    }

    throw (elastic::json::parse_error("term query must contain field"));
  }

  auto parse_terms_query(simdjson::ondemand::object terms_object) -> terms_query
  {
    terms_query result;
    for (auto field_result : terms_object)
    {
      simdjson::ondemand::field field;
      elastic::json::throw_if_error(std::move(field_result).get(field), "failed to read terms field");

      result.field = elastic::json::copy_string(elastic::json::read_key(field));

      simdjson::ondemand::array values;
      elastic::json::throw_if_error(field.value().get_array().get(values), "terms query value must be array");

      for (auto value_result : values)
      {
        simdjson::ondemand::value value;
        elastic::json::throw_if_error(std::move(value_result).get(value), "failed to read terms value");

        result.values.emplace_back(elastic::json::read_scalar_as_string(value, "terms value"));
      }

      if (result.field.empty())
      {
        throw (elastic::json::parse_error("terms query field is empty"));
      }

      if (result.values.empty())
      {
        throw (elastic::json::parse_error("terms query requires non-empty values array"));
      }

      return result;
    }

    throw (elastic::json::parse_error("terms query must contain field"));
  }
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
