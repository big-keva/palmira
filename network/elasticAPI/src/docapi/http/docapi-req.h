#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
#include "../../http/http-req.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::http
{
//-------------------------------------------------------------------------//
  struct document_get_options
  {
    bool source_enabled = true;

    std::vector<std::string> source_includes;
    std::vector<std::string> source_excludes;

    bool realtime = true;
    bool refresh = false;

    std::string routing;
    std::string preference;
    std::string stored_fields;
    std::string version;
    std::string version_type;

    auto as_zval() const noexcept -> mtc::zval;
  };

  struct index_document_request
  {
    std::string index;
    std::string id;
    std::string routing;
    std::string refresh;
    std::string pipeline;

    //!< Keeps document body.
    std::string body;
  };

  struct document_get_item
  {
    std::string index;
    std::string id;
    std::string routing;

    document_get_options options;
  };

  struct document_get_result
  {
    std::string index;
    std::string id;

    bool found = false;
    int status = 200;

    /*
     * Backend возвращает уже отфильтрованный _source.
     * HTTP-слой не должен повторно применять source filtering.
     */
    std::string source_json;

    /*
     * Если документ не может быть обработан индивидуально,
     * error_json содержит готовый JSON-объект ошибки.
     */
    std::string error_json;
  };

  struct multi_get_request
  {
    std::vector<document_get_item> documents;
  };

  struct multi_get_response
  {
    std::vector<document_get_result> documents;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses get options from query params.
   * @param params [in] - Query params to parse.
   * @return Parsed get options.
   */
  document_get_options parse_get_options(const elastic::http::query_params_t &params);

  /**
   * Makes index document request.
   * @param index [in] - Index name.
   * @param id [in] - Document ID.
   * @param body [in] - Document body.
   * @param params [in] - Query params.
   * @return Index document request.
   */
  palmira::InsertArgs make_index_request(std::string index, std::string id, std::string body, const elastic::http::query_params_t &params);

  /**
   * Makes index document request.
   * @param index [in] - Index name.
   * @param id [in] - Document ID.
   * @param body [in] - Document body.
   * @param params [in] - Query params.
   * @return Index document request.
   */
  palmira::UpdateArgs make_update_request(std::string index, std::string id, std::string body, const elastic::http::query_params_t &params);
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::http
