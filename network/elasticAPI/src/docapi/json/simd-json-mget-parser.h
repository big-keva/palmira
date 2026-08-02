#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
#include "../../json/parsers/simd-json-source-parser.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  struct mget_doc
  {
    //!< Keeps a name of index.
    std::string index;
    //!< Keeps a document id.
    std::string id;
    //!< Keeps a routing.
    std::string routing;
    //!< Keeps a source filter.
    elastic::json::source_filter source;
  };

  struct mget_request
  {
    //!< Keeps a list of documents.
    std::vector<mget_doc> docs;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses mget request.
   * @param body
   * @param default_index
   * @return
   */
  auto parse_mget_request(std::string_view body, std::string_view default_index) -> mget_request;

/**
   * Parses mget request.
   * @param index [in] - A name of index.
   * @param id [in] - A document id.
   * @param params [in] - A list of parameters.
   * @return
   */
  auto parse_mget_request(const std::string &index, const std::string &id, const mtc::zmap &params) -> mget_request;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
