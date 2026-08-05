#pragma once
//-------------------------------------------------------------------------//
#include <string_view>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  /**
   * Parses a document for updating into index.
   * @param body [in] - A document.
   * @param params [in] - A list of query parameters.
   * @return A document in search engine format.
   */
  auto parse_update_request(std::string_view body, const mtc::zmap &params) -> palmira::InsertArgs;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
