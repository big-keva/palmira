#include "../simd-json-match-query-parser.h"
//-------------------------------------------------------------------------//
#include "../../../json/simd-json-common.h"
#include "../../../json/simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  auto parse_match_query(simdjson::ondemand::object match_object) -> match_query
  {
    match_query result;

    for (auto field_result : match_object)
    {
      simdjson::ondemand::field field;
      elastic::json::throw_if_error(std::move(field_result).get(field), "failed to read match field");

      result.field = elastic::json::copy_string(elastic::json::read_key(field));
      simdjson::ondemand::value value = field.value();

      auto object_result = value.get_object();
      if (not object_result.error())
      {
        simdjson::ondemand::object params = object_result.value();
        for (auto param_result : params)
        {
          simdjson::ondemand::field param;
          elastic::json::throw_if_error(std::move(param_result).get(param), "failed to read match parameter");

          const std::string_view key = elastic::json::read_key(param);
          simdjson::ondemand::value param_value = param.value();

          if (key == "query")
          {
            result.query = elastic::json::read_scalar_as_string(param_value, "match.query");
          }
          else if (key == "operator")
          {
            result.operator_value = elastic::json::read_string_value(param_value, "match.operator must be string");
          }
          else if (key == "analyzer")
          {
            result.analyzer = elastic::json::read_string_value(param_value, "match.analyzer must be string");
          }
          else if (key == "fuzziness")
          {
            result.fuzziness = elastic::json::read_scalar_as_string(param_value, "match.fuzziness");
          }
          else if (key == "minimum_should_match")
          {
            result.minimum_should_match = elastic::json::read_scalar_as_string(param_value, "match.minimum_should_match");
          }
          else if (key == "zero_terms_query")
          {
            result.zero_terms_query = elastic::json::read_string_value(param_value, "match.zero_terms_query must be string");
          }
          else if (key == "lenient")
          {
            result.lenient = elastic::json::read_bool_value(param_value, "match.lenient must be boolean");
          }
          else if (key == "auto_generate_synonyms_phrase_query")
          {
            result.auto_generate_synonyms_phrase_query = elastic::json::read_bool_value(param_value, "match.auto_generate_synonyms_phrase_query must be boolean");
          }
          else if (key == "boost")
          {
            result.boost = elastic::json::read_double_value(param_value, "match.boost must be double");
          }
          else
          {
            /*
             * Elastic обычно допускает расширение параметров.
             * На первом этапе неизвестные параметры игнорируем.
             * Позже можно включить strict-режим.
             */
          }
        }
      }
      else
      {
        result.query = elastic::json::read_scalar_as_string(value, "match field value");
      }

      if (result.field.empty())
      {
        throw (elastic::json::parse_error("match query field is empty"));
      }

      if (result.query.empty())
      {
        throw (elastic::json::parse_error("match query requires query value"));
      }

      if (not result.operator_value.empty() &&
          result.operator_value != "or" &&
          result.operator_value != "and")
      {
        throw (elastic::json::parse_error(R"(match.operator must be "or" or "and")"));
      }

      if (not result.zero_terms_query.empty() &&
          result.zero_terms_query != "none" && result.zero_terms_query != "all")
      {
        throw (elastic::json::parse_error(R"(match.zero_terms_query must be "none" or "all")"));
      }
      return result;
    }

    throw (elastic::json::parse_error("match query must contain field"));
  }
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
