#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  /**
   * Parses get request.
   * @param index [in] - Default index.
   * @param id [in] - A document id.
   * @param params [in] - A list of query parameters.
   * @return A search engine arguments.
   */
  auto parse_get_request(std::string_view index, std::string_view id, const mtc::zmap &params) -> palmira::SearchArgs;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
