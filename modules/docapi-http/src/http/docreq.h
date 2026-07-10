/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          docreq.h
* - Created:       06/23/2026
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
#ifndef __DOCREQ_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __DOCREQ_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
//-------------------------------------------------------------------------//
#include <service.hpp>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  using query_params_t = std::unordered_map<std::string, std::string>;
//-------------------------------------------------------------------------//
  struct document_get_options {
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

  struct index_document_request {
    std::string index;
    std::string id;
    std::string routing;
    std::string refresh;
    std::string pipeline;

    //!< Keeps document body.
    std::string body;
  };

  struct document_get_item {
    std::string index;
    std::string id;
    std::string routing;

    document_get_options options;
  };

  struct document_get_result {
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

  struct multi_get_request {
    std::vector<document_get_item> documents;
  };

  struct multi_get_response {
    std::vector<document_get_result> documents;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses query string into a map of key-value pairs.
   * @param query [in] - A query string to parse.
   * @return A map of key-value pairs.
   */
  mtc::zmap parse_query(std::string_view query);

  /**
   * Parses get options from query params.
   * @param params [in] - Query params to parse.
   * @return Parsed get options.
   */
  document_get_options parse_get_options(const query_params_t &params);

  /**
   * Gets a value of parameter by its name.
   * @param params [in] - A list of request parameters.
   * @param name [in] - Parameter name.
   * @param default_value [in] - Default value to return if parameter is not found.
   * @return Parameter value or default value if parameter is not found.
   */
  auto get_query_param(const mtc::zmap &params, std::string_view name, std::optional<std::string> default_value = std::nullopt) -> std::optional<std::string>;

  /**
   * Makes index document request.
   * @param index [in] - Index name.
   * @param id [in] - Document ID.
   * @param body [in] - Document body.
   * @param params [in] - Query params.
   * @return Index document request.
   */
  palmira::InsertArgs make_index_request(std::string index, std::string id, std::string body, const query_params_t &params);

  /**
   * Makes index document request.
   * @param index [in] - Index name.
   * @param id [in] - Document ID.
   * @param body [in] - Document body.
   * @param params [in] - Query params.
   * @return Index document request.
   */
  palmira::UpdateArgs make_update_request(std::string index, std::string id, std::string body, const query_params_t &params);
//-------------------------------------------------------------------------//
} // namespace docapi::http
//-------------------------------------------------------------------------//
#endif // __DOCREQ_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
