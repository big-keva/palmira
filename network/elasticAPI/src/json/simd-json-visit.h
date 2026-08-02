#pragma once
//-------------------------------------------------------------------------//
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <mtc/interfaces.h>
#include <DeliriX/text-API.hpp>
//-------------------------------------------------------------------------//
#include "simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  enum class json_value_types
  {
    object_begin,
    object_end,
    array_begin,
    array_end,
    string,
    int64,
    uint64,
    double_value,
    boolean,
    datetime,
    null_value
  };

  struct json_visit_event
  {
    //!< Keeps a node path.
    std::string_view path;

    //!< Keeps a name of object.
    std::string_view name;

    //!< Keeps a type of value.
    json_value_types type = json_value_types::null_value;

    //!< Keeps a value.
    union
    {
      //!< Keeps a value as a string.
      std::string_view string_value;

      //!< Keeps a value as int64.
      std::int64_t int64_value;

      //!< Keeps a value as uint64.
      std::uint64_t uint64_value;

      //!< Keeps a value as uint64.
      std::uint64_t datetime_value;

      //!< Keeps a value as double.
      double double_value;

      //!< Keeps a value as boolean.
      bool bool_value;

      //!< Keeps a value of object.
      std::string_view object_value;
    } value;

    //!< Keeps index of document.
    //<note> NDJSON - the index of none null JSON line.
    std::uint64_t document_index = 0;

    /**
     * Ges presence of the object as a string.
     * @return The object as a string.
     */
    auto as_str() const noexcept -> std::string;
  };
//-------------------------------------------------------------------------//
  using json_visit_callback_t = std::function<bool(const json_visit_event &event)>;
//-------------------------------------------------------------------------//
  namespace detail
  {
//-------------------------------------------------------------------------//
    inline std::string append_object_path(std::string_view parent, std::string_view key)
    {
      std::string result;

      if (parent.empty())
      {
        result.reserve(key.size() + 1);
        result += '/';
        result += key;
      }
      else
      {
        result.reserve(parent.size() + key.size() + 1);
        result += parent;
        result += '/';
        result += key;
      }

      return result;
    }

    inline std::string append_array_path(std::string_view parent, std::size_t index)
    {
      std::string result(parent);

      result += '[';
      result += std::to_string(index);
      result += ']';

      return result;
    }

    template<typename Callback>
    inline auto emit_base(Callback &callback,
                          std::string_view path,
                          std::string_view name,
                          json_value_types type,
                          std::uint64_t document_index) -> void
    {
      callback(json_visit_event{
        .path = path,
        .name = name,
        .type = type,
        .value = {},
        .document_index = document_index
      });
    }

    template<typename Callback>
    inline auto emit_object(Callback &callback,
                            std::string_view path,
                            std::string_view name,
                            std::string_view value,
                            json_value_types type,
                            std::uint64_t document_index) -> void
    {
      callback(json_visit_event{
        .path = path,
        .name = name,
        .type = type,
        .value = {.object_value = value},
        .document_index = document_index
      });
    }

    template<typename Callback>
    inline auto emit_string(Callback &callback,
                            std::string_view path,
                            std::string_view name,
                            std::string_view value,
                            std::uint64_t document_index) -> void
    {
      callback(json_visit_event{
        .path = path,
        .name = name,
        .type = json_value_types::string,
        .value = {.string_value = value},
        .document_index = document_index,
      });
    }

    template<typename Callback>
    inline auto emit_int64(Callback &callback,
                           std::string_view path,
                           std::string_view name,
                           std::int64_t value,
                           std::uint64_t document_index) -> void
    {
      callback(json_visit_event{
        .path = path,
        .name = name,
        .type = json_value_types::int64,
        .value = {.int64_value = value},
        .document_index = document_index,
      });
    }

    template<typename Callback>
    inline auto emit_uint64(Callback &callback,
                            std::string_view path,
                            std::string_view name,
                            std::uint64_t value,
                            std::uint64_t document_index) -> void
    {
      callback(json_visit_event{
        .path = path,
        .name = name,
        .type = json_value_types::uint64,
        .value = {.uint64_value = value},
        .document_index = document_index,
      });
    }

    template<typename Callback>
    inline auto emit_double(Callback &callback,
                            std::string_view path,
                            std::string_view name,
                            double value,
                            std::uint64_t document_index) -> void
    {
      callback(json_visit_event{
        .path = path,
        .name = name,
        .type = json_value_types::double_value,
        .value = {.double_value = value},
        .document_index = document_index,
      });
//-------------------------------------------------------------------------//
  } // namespace detail
//-------------------------------------------------------------------------//
  template<typename Callback>
  inline auto emit_bool(Callback &callback,
                        std::string_view path,
                        std::string_view name,
                        bool value,
                        std::uint64_t document_index) -> void
  {
    callback(json_visit_event{
      .path = path,
      .name = name,
      .type = json_value_types::boolean,
      .value = {.bool_value = value},
      .document_index = document_index,
    });
  }
//-------------------------------------------------------------------------//
  template<typename Callback>
  auto walkValue(simdjson::ondemand::value value,
                 const mtc::api<DeliriX::IText>& text,
                 std::string_view path,
                 std::string_view name,
                 Callback &callback,
                 std::uint64_t document_index) -> void;

  template<typename Callback>
  auto walkObject(simdjson::ondemand::object object,
                  std::string_view path,
                  std::string_view name,
                  Callback &callback,
                  std::uint64_t document_index) -> void
  {
    // Raising OBJECT_BEGIN event.
    //<???> emit_object(callback, text->AddMarkupTag(name), path, name, object.raw_json(), json_value_types::object_begin, document_index);

    for (auto field_result : object)
    {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read object field");

      std::string_view key;
      throw_if_error(field.unescaped_key(false).get(key), "failed to read key");

      auto child_value = field.value();
      auto child_path = append_object_path(path, key);

      walkValue(child_value, child_path, key, callback, document_index);
    }

    // Raising OBJECT_END event.
    //<???> emit_base(callback, path, name, json_value_types::object_end, document_index);
  }

  template<typename Callback>
  auto walkArray(simdjson::ondemand::array array,
                 std::string_view path,
                 std::string_view name,
                 Callback &callback,
                 std::uint64_t document_index) -> void
  {
    // Raising OBJECT_BEGIN event.
    //<???> emit_base(callback, path, name, json_value_types::array_begin, document_index);

    std::size_t index = 0;

    for (auto element_result : array)
    {
      simdjson::ondemand::value element;
      throw_if_error(element_result.get(element), "failed to read array element");

      auto child_path = append_array_path(path, index);
      walkValue(element, child_path, name, callback, document_index);

      ++index;
    }

    // Raising OBJECT_END event.
    //<???> emit_base(callback, path, name, json_value_types::array_end, document_index);
  }

  template<typename Callback>
  auto walkNumber(simdjson::ondemand::value value,
                  std::string_view path,
                  std::string_view name,
                  Callback &callback,
                  std::uint64_t document_index) -> void
  {
    simdjson::ondemand::number number;
    throw_if_error(value.get_number().get(number), "failed to read JSON number");

    switch (number.get_number_type())
    {
    case simdjson::ondemand::number_type::signed_integer:
      emit_int64(callback, path, name, number.get_int64(), document_index);
      break;
    case simdjson::ondemand::number_type::unsigned_integer:
      emit_uint64(callback, path, name, number.get_uint64(), document_index);
      break;
    case simdjson::ondemand::number_type::floating_point_number:
      emit_double(callback, path, name, number.get_double(), document_index);
      break;
    }
  }
//-------------------------------------------------------------------------//
  template <typename Callback>
  auto walkValue(simdjson::ondemand::value value,
                 std::string_view path,
                 std::string_view name,
                 Callback &callback,
                 std::uint64_t document_index) -> void
  {
    simdjson::ondemand::json_type type;
    throw_if_error(value.type().get(type), "failed to detect JSON value type");

    switch (type)
    {
      case simdjson::ondemand::json_type::object: {
        simdjson::ondemand::object object;
        throw_if_error(value.get_object().get(object), "failed to get JSON object");

        walkObject(object, path, name, callback, document_index);
        break;
      }
      case simdjson::ondemand::json_type::array: {
        simdjson::ondemand::array array;
        throw_if_error(value.get_array().get(array), "failed to get JSON array");

        walkArray(array, path, name, callback, document_index);
        break;
      }
      case simdjson::ondemand::json_type::string: {
        std::string_view result;
        throw_if_error(value.get_string().get(result), "failed to get JSON string");

        emit_string(callback, path, name, result, document_index);
        break;
      }
      case simdjson::ondemand::json_type::number:
        walkNumber(value, path, name, callback, document_index);
        break;
      case simdjson::ondemand::json_type::boolean: {
        bool result = false;
        throw_if_error(value.get_bool().get(result), "failed to get JSON boolean");

        emit_bool(callback, path, name, result, document_index);
        break;
      }
      case simdjson::ondemand::json_type::null:
        emit_base(callback, path, name, json_value_types::null_value, document_index);
        break;
    }
  }

  template<typename Callback>
  auto visit_single_json_document(std::string_view json, DeliriX::IText &text, Callback &callback, std::uint64_t document_index) -> void
  {
    /*
     * simdjson требует padding после входного буфера.
     * padded_string делает безопасную копию.
     *
     * Для максимальной производительности HTTP body collector позже можно
     * сделать padded-буфер сразу при накоплении тела запроса.
     */
    simdjson::padded_string padded_json(json);
    simdjson::ondemand::value root;
    simdjson::ondemand::document document;
    //<!!!> parser is not thread-safe.
    thread_local simdjson::ondemand::parser parser;

    throw_if_error(parser.iterate(padded_json).get(document), "failed to parse JSON document");
    throw_if_error(document.get_value().get(root), "failed to read root JSON value");

    walkValue(root, mtc::api<DeliriX::IText>(&text), {}, {}, callback, document_index);
  }

  inline auto is_whitespace_only(std::string_view line) -> bool
  {
    return std::ranges::all_of(std::begin(line), std::end(line), [](auto c) -> bool {
      return c != ' ' && c != '\t' && c != '\r' && c != '\n' ? false : true;
    });
  }
//-------------------------------------------------------------------------//
  } // namespace detail
//-------------------------------------------------------------------------//
  template<typename Callback>
  auto visit_json_cb(std::string_view json, Callback &&callback) -> void
  {
    detail::visit_single_json_document(json, std::forward<Callback>(callback), 0);
  }

  template<typename Callback>
  auto visit_ndjson_cb(std::string_view ndjson, Callback &&callback) -> void
  {
    auto callback_holder = std::forward<Callback>(callback);
    std::uint64_t document_index = 0;
    std::size_t offset = 0;

    while (offset < ndjson.size())
    {
      std::string_view line;
      const std::size_t line_end = ndjson.find('\n', offset);
      if (line_end == std::string_view::npos)
      {
        line = ndjson.substr(offset);
        offset = ndjson.size();
      }
      else
      {
        line = ndjson.substr(offset, line_end - offset);
        offset = line_end + 1;
      }

      if (not line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }

      if (line.empty() || detail::is_whitespace_only(line))
      {
        continue;
      }

      detail::visit_single_json_document(line, callback_holder, document_index);

      ++document_index;
    }
  }
//-------------------------------------------------------------------------//
  auto to_string_view(json_value_types type) -> std::string_view;
//-------------------------------------------------------------------------//
} // namespace elastic::json
