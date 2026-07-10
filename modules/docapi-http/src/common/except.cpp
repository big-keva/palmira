#include "except.h"
//-------------------------------------------------------------------------//
namespace docapi::common {
//-------------------------------------------------------------------------//
  json_parse_error::json_parse_error(const std::string &msg) : std::invalid_argument(msg) {
  }
//-------------------------------------------------------------------------//
} // namespace docapi::common
