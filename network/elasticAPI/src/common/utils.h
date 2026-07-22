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
#ifndef __UTILS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __UTILS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include "libmorph/xmorph/codepages.hpp"

#include <cstdint>
#include <string>
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
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
#endif // __UTILS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
