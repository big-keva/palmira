/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          simd-json-errors.h
* - Created:       07/08/2026
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
#ifndef __JSON_ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __JSON_ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  struct parse_error final : public std::runtime_error
  {
    /**
     * Constructor.
     * @param message [in] - Error message.
     */
    explicit parse_error(const std::string &message);
  };
//-------------------------------------------------------------------------//
  auto throw_if_error(simdjson::error_code error, std::string_view context) -> void;
//-------------------------------------------------------------------------//
} // namespace elastic::json
//-------------------------------------------------------------------------//
#endif // __JSON_ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
