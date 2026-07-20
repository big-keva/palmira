/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          logger.h
* - Created:       07/20/2026
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
#ifndef __LOGGER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __LOGGER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <cstdint>
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
namespace elastic::logger
{
//-------------------------------------------------------------------------//
  enum class severities : std::uint8_t
  {
    none = 0,
    error,
    warning,
    info,
    debug,
    trace
  };
//-------------------------------------------------------------------------//
  /**
   * Converts a severity into string.
   * @param severity [in] - A debug level.
   * @return A debug level as a string.
   */
  auto to_string_view(severities severity) -> std::string_view;

  /**
   * Gets a severity from a string.
   * @param severity [in] - A severity as a string.
   * @return A severity.
   */
  auto severity_from(std::string_view severity) -> severities;
//-------------------------------------------------------------------------//
  /**
   * Initializes a logger.
   * @param fname [in] - A name of file.
   */
  auto init_logger(const std::string &fname) -> void;

  /**
   * Destroys a logger.
   */
  auto destroy_logger() -> void;

  /**
   * Sets a debug level.
   * @param debug_level [in] - A debug level.
   */
  auto set_debug_level(severities debug_level) -> void;

  /**
   * Sets a debug level.
   * @param debug_level [in] - A debug level as a string.
   */
  auto set_debug_level(const char *debug_level) -> void;

  /**
   * Gets a debug level.
   * @return A debug level.
   */
  auto get_debug_level() -> severities;
//-------------------------------------------------------------------------//
  auto to_log(severities level, const char *file, int line, const char *func_name, const char *fmt, ...) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::logger
//-------------------------------------------------------------------------//
#define LOG_E_C(...) to_log(elastic::logger::severities::error,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_W_C(...) to_log(elastic::logger::severities::warning, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_I_C(...) to_log(elastic::logger::severities::info,    __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_D_C(...) to_log(elastic::logger::severities::debug,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_T_C(...) to_log(elastic::logger::severities::trace,   __FILE__, __LINE__, __func__, __VA_ARGS__)

/*
#define LOG_E_C(...) to_log(elastic::logger::severities::error,   __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define LOG_W_C(...) to_log(elastic::logger::severities::warning, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define LOG_I_C(...) to_log(elastic::logger::severities::info,    __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define LOG_D_C(...) to_log(elastic::logger::severities::debug,   __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define LOG_T_C(...) to_log(elastic::logger::severities::trace,   __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
*/
//-------------------------------------------------------------------------//
#endif // __LOGGER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
