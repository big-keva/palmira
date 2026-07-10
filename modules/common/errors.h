/*!==========================================================================
* \file
* - Program:       modules-common
* - File:          errors.h
* - Created:       07/05/2026
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
#ifndef __ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace palmira::modules {
//-------------------------------------------------------------------------//
  class palmira_error : public std::runtime_error {
    using base_class = std::runtime_error;
    //!< Keeps error code.
    int ecode = 0;

  public:
    /**
     * Constructor.
     * @param msg [in] - Error message.
     */
    explicit palmira_error(const char *msg);

    /**
     * Constructor.
     * @param code [in] - Error code.
     * @param msg [in] - Error message.
     */
    explicit palmira_error(int code, const char *msg);

    /**
     * Gets error code.
     * @return Error code.
     * @throw None.
     */
    [[nodiscard]] auto code() const noexcept -> int {
      return this->ecode;
    }
  };
//-------------------------------------------------------------------------//
  auto check_errors(const mtc::zmap &zmap) -> void;
//-------------------------------------------------------------------------//
} // namespace palmira::modules
//-------------------------------------------------------------------------//
#endif // __ERRORS_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
