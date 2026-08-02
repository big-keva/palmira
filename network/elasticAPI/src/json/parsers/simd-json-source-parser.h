#pragma once
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  struct source_filter
  {
    //!< Keeps a flag of using filter.
    bool enabled = true;

    //!< Keeps a list of includes.
    std::vector<std::string> includes;

    //!< Keeps a list of excludes.
    std::vector<std::string> excludes;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses a source object.
   * @param object [in] - A source object.
   * @param source [out] - A source.
   */
  auto parse_source_object(simdjson::ondemand::object object, source_filter &source) -> void;

  /**
   * Parses a source value.
   * @param value [in] - A source value.
   * @param source [in, out] - A source.
   */
  auto parse_source_value(simdjson::ondemand::value value, source_filter &source) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::json
