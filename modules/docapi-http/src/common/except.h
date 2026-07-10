/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          docexcept.h
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
#ifndef __DOCEXCEPT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __DOCEXCEPT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string>
#include <stdexcept>
//-------------------------------------------------------------------------//
namespace docapi::common {
//-------------------------------------------------------------------------//
  struct json_parse_error final : public std::invalid_argument {
    /**
     * Constructor.
     * @param msg [in] - Error message.
     */
    explicit json_parse_error(const std::string &msg);
  };
//-------------------------------------------------------------------------//
} // namespace docapi::common
//-------------------------------------------------------------------------//
#endif // __DOCEXCEPT_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
