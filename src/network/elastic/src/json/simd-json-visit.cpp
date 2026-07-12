#include "simd-json-visit.h"
//-------------------------------------------------------------------------//
#include <sstream>
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  auto json_visit_event::as_str() const noexcept -> std::string
  {
    std::ostringstream buffer;
    const auto value_type = to_string_view(this->type);

    buffer << "path=";
    buffer.write(this->path.data(), this->path.size());
    buffer << ", ";
    buffer << "name=";
    buffer.write(this->name.data(), this->name.size());
    buffer << ", ";

    if (this->type == json_value_types::string)
    {
      buffer << "value=";
      buffer.write(this->value.string_value.data(),
                   this->value.string_value.size());
      buffer << ", ";
    }
    else if (this->type == json_value_types::boolean)
    {
      buffer << "value=" << std::boolalpha << this->value.bool_value << ", ";
    }
    else if (this->type == json_value_types::int64)
    {
      buffer << "value=" << this->value.int64_value << ", ";
    }
    else if (this->type == json_value_types::uint64)
    {
      buffer << "value=" << this->value.uint64_value << ", ";
    }
    else if (this->type == json_value_types::double_value)
    {
      buffer << "value=" << this->value.double_value << ", ";
    }
    else if (this->type == json_value_types::datetime)
    {
      buffer << "value=" << this->value.datetime_value << ", ";
    }
    buffer << "type=";
    buffer.write(value_type.data(), value_type.size());
    return buffer.str();
  }
//-------------------------------------------------------------------------//
  auto to_string_view(json_value_types type) -> std::string_view
  {
    switch (type)
    {
    case json_value_types::object_begin:
      return "object_begin";
    case json_value_types::object_end:
      return "object_end";
    case json_value_types::array_begin:
      return "array_begin";
    case json_value_types::array_end:
      return "array_end";
    case json_value_types::string:
      return "string";
    case json_value_types::int64:
      return "int64";
    case json_value_types::uint64:
      return "uint64";
    case json_value_types::double_value:
      return "double";
    case json_value_types::boolean:
      return "boolean";
    case json_value_types::datetime:
      return "datetime";
    case json_value_types::null_value:
      return "null";
    }
    return "unknown";
  }
//-------------------------------------------------------------------------//
} // namespace docapi::json
