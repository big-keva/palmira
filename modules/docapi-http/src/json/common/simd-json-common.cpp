#include "../simd-json-common.h"
//-------------------------------------------------------------------------//
#include "../simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace docapi::json {
//-------------------------------------------------------------------------//
  auto detect_query_kind(simdjson::ondemand::object query_object) -> query_kinds {
    /*
     * Query DSL на первом этапе не типизируем полностью.
     * Только определяем верхнеуровневый тип.
     */
    for (auto field_result: query_object) {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read query field");

      const std::string_view key = read_key(field);
      if (key == "match_all") { return query_kinds::match_all; }
      if (key == "match_none") { return query_kinds::match_none; }
      if (key == "term") { return query_kinds::term; }
      if (key == "terms") { return query_kinds::terms; }
      if (key == "range") { return query_kinds::range; }
      if (key == "bool") { return query_kinds::bool_query; }
      if (key == "as_tree") { return query_kinds::as_tree; }

      return query_kinds::raw;
    }
    return query_kinds::none;
  }
//-------------------------------------------------------------------------//
  auto get_thread_parser_context() -> parser_context & {
    thread_local parser_context context;
    return context;
  }

  auto validate_json_object(std::string_view body) -> void {
    if (body.empty()) {
      throw (parse_error("document body is empty"));
    }

    simdjson::padded_string padded(body);
    simdjson::ondemand::document document;
    auto &context = get_thread_parser_context();
    throw_if_error(context.parser.iterate(padded).get(document), "failed to parse JSON");

    simdjson::ondemand::object root;
    throw_if_error(document.get_object().get(root), "JSON body must be object");
  }

  auto copy_string(std::string_view value) -> std::string {
    return {value.data(), value.size()};
  }

  auto read_string_view(simdjson::ondemand::value value, std::string_view context) -> std::string_view {
    std::string_view result;
    throw_if_error(value.get_string().get(result), context);
    return result;
  }

  auto read_scalar_as_string(simdjson::ondemand::value value, std::string_view context) -> std::string {
    std::string_view string_value;

    if (not value.get_string().get(string_value)) {
      return copy_string(string_value);
    }

    simdjson::ondemand::number number;
    if (not value.get_number().get(number)) {
      switch (number.get_number_type()) {
        case simdjson::ondemand::number_type::signed_integer:
          return std::to_string(number.get_int64());
        case simdjson::ondemand::number_type::unsigned_integer:
          return std::to_string(number.get_uint64());
        case simdjson::ondemand::number_type::floating_point_number:
          return std::to_string(number.get_double());
      }
    }

    bool bool_value = false;
    if (not value.get_bool().get(bool_value)) {
      return bool_value ? "true" : "false";
    }

    throw (parse_error(std::string(context) + ": expected scalar value"));
  }

  auto read_key(simdjson::ondemand::field& field) -> std::string_view {
    std::string_view key;
    throw_if_error(field.unescaped_key(false).get(key), "failed to read object key");

    return key;
  }

  auto read_double(simdjson::ondemand::value value, std::string_view context) -> double {
    double result = 0.0;
    throw_if_error(value.get_double().get(result), context);

    return result;
  }

  std::string_view read_string(simdjson::ondemand::value value) {
    std::string_view result = {};
    throw_if_error(value.get_string().get(result), "failed to read string");

    return result;
  }

  auto read_double_value(simdjson::ondemand::value value, std::string_view context) -> double {
    double result = 0.0;
    throw_if_error(value.get_double().get(result), context);
    return result;
  }

  auto read_string_value(simdjson::ondemand::value value, std::string_view context) -> std::string {
    std::string_view result;
    throw_if_error(value.get_string().get(result), context);

    return copy_string(result);
  }

  std::uint64_t read_non_negative_integer(simdjson::ondemand::value value, std::string_view field_name) {
    simdjson::ondemand::number number;
    throw_if_error(value.get_number().get(number), std::string("failed to read number field: ") + std::string(field_name));

    switch (number.get_number_type()) {
      case simdjson::ondemand::number_type::signed_integer: {
        const auto v = number.get_int64();

        if (v < 0) {
          throw (parse_error(std::string(field_name) + " must be non-negative"));
        }
        return static_cast<std::uint64_t>(v);
      }
      case simdjson::ondemand::number_type::unsigned_integer: {
        return number.get_uint64();
      }
      case simdjson::ondemand::number_type::floating_point_number: {
        throw (parse_error(std::string(field_name) + " must be integer"));
      }
    }

    throw (parse_error("unknown number type"));
  }

  bool read_bool(simdjson::ondemand::value value, std::string_view field_name) {
    bool result = false;
    throw_if_error(value.get_bool().get(result), std::string("failed to read bool field: ") + std::string(field_name));

    return result;
  }

  auto read_bool_value(simdjson::ondemand::value value, std::string_view context) -> bool {
    bool result = false;
    throw_if_error(value.get_bool().get(result), context);
    return result;
  }

  void parse_string_array(simdjson::ondemand::array array, std::vector<std::string> &output) {
    for (auto item_result : array) {
      simdjson::ondemand::value item;
      throw_if_error(std::move(item_result).get(item), "failed to read array item");

      output.emplace_back(copy_string(read_string(item)));
    }
  }

  auto parse_string_array(simdjson::ondemand::value value, std::vector<std::string> &output, std::string_view context) -> void {
    simdjson::ondemand::array array;
    throw_if_error(value.get_array().get(array), context);

    for (auto item_result : array) {
      simdjson::ondemand::value item;
      throw_if_error(std::move(item_result).get(item), context);

      output.emplace_back(read_scalar_as_string(item, context));
    }
  }
//-------------------------------------------------------------------------//
} // namespace docapi::json
