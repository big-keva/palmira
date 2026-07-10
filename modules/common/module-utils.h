/*!==========================================================================
* \file
* - Program:       modules-common
* - File:          module-utils.h
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
#ifndef __MODULE_UTILS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __MODULE_UTILS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <cstdint>
//-------------------------------------------------------------------------//
namespace palmira::modules {
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
//-------------------------------------------------------------------------//
} // namespace palmira::modules
//-------------------------------------------------------------------------//
#endif // __MODULE_UTILS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
