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
  struct parse_error final : public std::runtime_error
  {
    /**
     * Constructor.
     * @param message [in] - Error message.
     */
    explicit parse_error(const std::string &message);
  };
//-------------------------------------------------------------------------//
  /**
   * Checks error object on error.
   * @param error [in] - Error object.
   * @param context [in] - A context error.
   * @throw parse_error - Error.
   */
  auto throw_if_error(simdjson::error_code error, std::string_view context) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::json
