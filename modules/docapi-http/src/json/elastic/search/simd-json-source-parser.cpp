#include "simd-json-source-parser.h"
//-------------------------------------------------------------------------//
#include "../../simd-json-common.h"
#include "../../simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic::search
{
//-------------------------------------------------------------------------//
auto parse_source_object(simdjson::ondemand::object object,
                         source_filter &source) -> void
{
  for (auto field_result : object)
  {
    simdjson::ondemand::field field;
    throw_if_error(std::move(field_result).get(field),
                   "failed to read _source field");

    const std::string_view key = read_key(field);
    simdjson::ondemand::value value = field.value();

    if (key == "includes" || key == "include")
    {
      simdjson::ondemand::array array;
      throw_if_error(value.get_array().get(array),
                     "_source.includes must be array");

      parse_string_array(array, source.includes);
    }
    else if (key == "excludes" || key == "exclude")
    {
      simdjson::ondemand::array array;
      throw_if_error(value.get_array().get(array),
                     "_source.excludes must be array");

      parse_string_array(array, source.excludes);
    }
  }
}

auto parse_source_value(simdjson::ondemand::value value, source_filter &source)
    -> void
{
  /*
   * Важно:
   * Здесь НЕ вызываем value.type().
   * Работаем по допустимым формам _source.
   */

  auto bool_result = value.get_bool();

  if (not bool_result.error())
  {
    source.enabled = bool_result.value();
    return;
  }

  auto array_result = value.get_array();
  if (not array_result.error())
  {
    source.enabled = true;
    simdjson::ondemand::array array = array_result.value();
    parse_string_array(array, source.includes);
    return;
  }

  auto object_result = value.get_object();
  if (not object_result.error())
  {
    source.enabled = true;
    simdjson::ondemand::object object = object_result.value();
    parse_source_object(object, source);
    return;
  }

  throw parse_error("_source must be boolean, array or object");
}
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic::search
