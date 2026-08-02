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
