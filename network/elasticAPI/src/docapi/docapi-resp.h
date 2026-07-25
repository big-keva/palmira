/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          docapi-resp.h
* - Created:       06/24/2026
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
#ifndef __DOCAPI_RESP_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __DOCAPI_RESP_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace elastic::http::docapi
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

  /**
   * Makes a response of searching a document.
   * @param ctx [in] - HTTP context.
   * @param resp [in] - A response.
   * @return A response in JSON format.
   */
  template<bool SSL>
  auto send_search_response(response_context<SSL> *ctx, const mtc::zmap &resp) -> std::string;
//-------------------------------------------------------------------------//
}// namespace elastic::http::docapi
//-------------------------------------------------------------------------//
#endif // __DOCAPI_RESP_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
