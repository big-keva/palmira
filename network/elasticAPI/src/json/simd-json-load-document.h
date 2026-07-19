/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          simd-json-load-document.h
* - Created:       07/19/2026
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
#ifndef __SIMD_JSON_LOAD_DOCUMENT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SIMD_JSON_LOAD_DOCUMENT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
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
namespace elastic::json
{
//-------------------------------------------------------------------------//
  /**
   * Loads JSON document into IText.
   * @param body [in] - JSON value.
   * @param onadd [in] - A callback method.
   */
  auto load_document(std::string_view body, std::function<mtc::api<DeliriX::IText>()> onadd) -> void;

  /**
   * Loads JSON document into IText.
   * @param value [in] - JSON value.
   * @param onadd [in] - A callback method.
   */
  auto load_document(simdjson::ondemand::value value, std::function<mtc::api<DeliriX::IText>()> onadd) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::json
//-------------------------------------------------------------------------//
#endif // __SIMD_JSON_LOAD_DOCUMENT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
