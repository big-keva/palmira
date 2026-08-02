#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  struct parser_context
  {
    simdjson::ondemand::parser parser;

    void reset() noexcept
    {
      /*
       * simdjson::ondemand::parser переиспользуется.
       * Важно: не использовать один parser параллельно из разных потоков.
       */
    }
  };

  //!< Supported query kinds.
  enum class query_kinds
  {
    none,
    raw,
    match_all,
    match_none,
    match,
    term,
    terms,
    range,
    bool_query,
    as_tree
  };
//-------------------------------------------------------------------------//
  auto detect_query_kind(simdjson::ondemand::object query_object) -> query_kinds;
//-------------------------------------------------------------------------//
  /**
   * Gets a parser context in thread local.
   * @return A parser context in thread local.
   */
  auto get_thread_parser_context() -> parser_context &;

  /**
   * Validates JSON object.
   * @param body [in] - JSON object.
   */
  auto validate_json_object(std::string_view body) -> void;

  /**
   * Gets a copy of string.
   * @param value [in] - A string.
   * @return A copy string.
   */
  auto copy_string(std::string_view value) -> std::string;

  /**
   * Reads a string from value.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @return A string.
   */
  auto read_string_view(simdjson::ondemand::value value, std::string_view context) -> std::string_view;

  /**
   * Reads a scalar from value as a string.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @return A scalar from value as a string.
   */
  auto read_scalar_as_string(simdjson::ondemand::value value, std::string_view context) -> std::string;

  /**
   * Reads a double from value.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @return A double.
   */
  auto read_double(simdjson::ondemand::value value, std::string_view context) -> double;

  /**
   * Reads a double from value.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @return A double.
   */
  auto read_double_value(simdjson::ondemand::value value, std::string_view context) -> double;

  /**
   * Reads a string from value.
   * @param value [in] - A value.
   * @return A string.
   */
  auto read_string(simdjson::ondemand::value value) -> std::string_view;

  /**
   * Reads a string from value.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @return A string.
   */
  auto read_string_value(simdjson::ondemand::value value, std::string_view context) -> std::string;

  /**
   * Reads a positive number from value.
   * @param value [in] - A value.
   * @param field_name [in] - A name of field.
   * @return A number.
   */
  auto read_non_negative_integer(simdjson::ondemand::value value, std::string_view field_name) -> std::uint64_t;

  /**
   * Reads a boolean from value.
   * @param value [in] - A value.
   * @param field_name [in] - A name of field.
   * @return A boolean.
   */
  auto read_bool(simdjson::ondemand::value value, std::string_view field_name) -> bool;

  /**
   * Reads a boolean from value.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @return A boolean.
   */
  auto read_bool_value(simdjson::ondemand::value value, std::string_view context) -> bool;

  /**
   * Reads a key from field.
   * @param field [in] - A field.
   * @return A key.
   */
  auto read_key(simdjson::ondemand::field &field) -> std::string_view;
//-------------------------------------------------------------------------//
  /**
   * Parses an array from JSON array object.
   * @param array [in] - JSON array.
   * @param output [in, out] - A list of strings.
   */
  auto parse_string_array(simdjson::ondemand::array array, std::vector<std::string> &output) -> void;

  /**
   * Parses an array from value.
   * @param value [in] - A value.
   * @param context [in] - A context.
   * @param output [out] - A list of strings.
   */
  auto parse_string_array(simdjson::ondemand::value value,
                          std::string_view context,
                          std::vector<std::string> &output) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::json
