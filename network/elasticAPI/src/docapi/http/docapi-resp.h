#pragma once
//-------------------------------------------------------------------------//
#include <string>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
#include "../../http/http-resp.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::http
{
//-------------------------------------------------------------------------//
  /**
   * Makes a response of indexing a document.
   * @param resp [in] - A response.
   * @return A response in JSON format.
   */
  auto make_index_response(const mtc::zmap &resp) -> std::string;

  /**
   * Makes a response of updating a document.
   * @param resp [in] - A response.
   * @return A response in JSON format.
   */
  auto make_update_response(const mtc::zmap &resp) -> std::string;

  /**
   * Makes a response of removing a document.
   * @param resp [in] - A response.
   * @return A response in JSON format.
   */
  auto make_remove_response(const mtc::zmap &resp) -> std::string;

  template<bool SSL>
  auto send_index_json_response(uWS::HttpResponse<SSL> *ctx, const mtc::zmap &resp, std::string_view index, std::string_view docid) -> void;

  template<bool SSL>
  auto send_update_json_response(uWS::HttpResponse<SSL> *ctx, const mtc::zmap &resp, std::string_view index, std::string_view docid) -> void;

  template<bool SSL>
  auto send_get_json_response(uWS::HttpResponse<SSL> *ctx, const mtc::zmap &resp, std::string_view index, std::string_view docid) -> void;

  template<bool SSL>
  auto send_del_json_response(uWS::HttpResponse<SSL> *ctx, const mtc::zmap &resp, std::string_view index, std::string_view docid) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::http
