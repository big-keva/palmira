#include "../simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace docapi::json
{
//-------------------------------------------------------------------------//
parse_error::parse_error(const std::string &message) : std::runtime_error(message)
{
}
//-------------------------------------------------------------------------//
auto throw_if_error(simdjson::error_code error, std::string_view context) -> void
{
  if (error)
  {
    throw(parse_error(std::string(context) + ": " + simdjson::error_message(error)));
  }
}
//-------------------------------------------------------------------------//
} // namespace docapi::json
