/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          simd-json-common.h
* - Created:       07/08/2026
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
#ifndef __JSON_COMMON_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __JSON_COMMON_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
namespace docapi::json
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

auto read_string_view(simdjson::ondemand::value value, std::string_view context) -> std::string_view;

auto read_scalar_as_string(simdjson::ondemand::value value, std::string_view context) -> std::string;

auto read_key(simdjson::ondemand::field &field) -> std::string_view;

auto read_double(simdjson::ondemand::value value, std::string_view context) -> double;

auto read_string(simdjson::ondemand::value value) -> std::string_view;

auto read_double_value(simdjson::ondemand::value value, std::string_view context) -> double;

auto read_string_value(simdjson::ondemand::value value, std::string_view context) -> std::string;

auto read_non_negative_integer(simdjson::ondemand::value value, std::string_view field_name) -> std::uint64_t;

auto read_bool(simdjson::ondemand::value value, std::string_view field_name) -> bool;

auto read_bool_value(simdjson::ondemand::value value, std::string_view context) -> bool;

auto parse_string_array(simdjson::ondemand::array array, std::vector<std::string> &output) -> void;

auto parse_string_array(simdjson::ondemand::value value,
                        std::vector<std::string> &output,
                        std::string_view context) -> void;
//-------------------------------------------------------------------------//
} // namespace docapi::json
//-------------------------------------------------------------------------//
#endif // __JSON_COMMON_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
