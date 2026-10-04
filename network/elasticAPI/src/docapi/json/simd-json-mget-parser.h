#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  /**
   * Parses mget request.
   * @param body [in] - A request body.
   * @param default_index [in] - Default index.
   * @return A search engine arguments.
   */
  auto parse_mget_request(std::string_view body, std::string_view default_index) -> palmira::SearchArgs;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
