#pragma once
//-------------------------------------------------------------------------//
#include <string_view>
#include <functional>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
#include <DeliriX/text-API.hpp>
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  /**
   * Parses a document for adding into index.
   * @param body [in] - A document.
   * @param params [in] - A list of query parameters.
   * @return A document in search engine format.
   */
  auto parse_index_request(std::string_view body, const mtc::zmap &params) -> palmira::InsertArgs;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
