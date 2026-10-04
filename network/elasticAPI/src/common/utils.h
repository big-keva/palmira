/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          utils.h
* - Created:       06/30/2026
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
#include <cstdint>
#include <string>
#include <functional>
//-------------------------------------------------------------------------//
#include <moonycode/codes.h>
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  /**
   * Parses a size from string.
   * @param size [in] - A size as a string.
   * @return A number of size.
   */
  auto parse_size(const std::string &size) -> size_t;

  /**
   * Parses a timeout as a string.
   * @param timeout [in] - A timeout as a string.
   * @return A number of seconds.
   */
  auto parse_timeout(const std::string &timeout) -> size_t;

  /**
   * Makes unique id.
   * @param size [in] - A size of unique id.
   * @return UID.
   */
  auto make_uid(std::uint8_t size = 16) -> std::string;

  /**
   * Converts a string into UTF8.
   * @param src [in] - A source string.
   * @return A string in UTF8.
   */
  auto to_utf8(const codepages::widestring &src) -> std::string;

  /**
   * Splits a string by separated string.
   * @param text [in] - A text.
   * @param sep [in] - A separate string.
   * @param onsubstr [in] - A callback method.
   */
  auto split_string(const std::string_view &text, const std::string_view &sep, std::function<void(std::string_view)> onsubstr) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic
