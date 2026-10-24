#pragma once
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <optional>
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  struct range_query
  {
    //!< Keeps a name of field.
    std::string field;

    std::optional<std::string> gt;
    std::optional<std::string> gte;
    std::optional<std::string> lt;
    std::optional<std::string> lte;

    std::string format;
    std::string time_zone;
    std::string relation;

    double boost = 1.0;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses a range query.
   * @param range_object [in] - A range object.
   * @return A range query.
   */
  auto parse_range_query(simdjson::ondemand::object range_object) -> range_query;
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
