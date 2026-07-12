/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          simd-json-mget-parser.h
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
#ifndef __SIMD_JSON_MGET_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_MGET_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
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
namespace elastic::json::docapi
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
    source_filter source;
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
} // namespace elastic::json::docapi
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_MGET_PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
