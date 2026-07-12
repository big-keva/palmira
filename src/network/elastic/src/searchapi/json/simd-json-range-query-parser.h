/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          simd-json-range-query-parser.h
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
#ifndef __SIMD_JSON_RANGE_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_RANGE_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
namespace elastic::json::searchapi
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
} // namespace elastic::json::searchapi
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_RANGE_QUERY_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
