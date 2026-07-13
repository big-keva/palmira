/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          profiler.h
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
#ifndef __PROFILER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __PROFILER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <chrono>
#include <functional>
#include <string>
//-------------------------------------------------------------------------//
namespace elastic
{
//-------------------------------------------------------------------------//
  class profiler final
  {
  public:
    using duration_t = std::chrono::duration<double, std::milli>;
    using onevent_t = std::function<void(duration_t)>;

    //!< Default event handler.
    static auto event_default(duration_t) -> void;

  private:
    //!< Setting a time started marker.
    const std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();

    //!< Keeps a callback function.
    onevent_t onevent;

  public:
    /**
     * Constructor.
     * @param onevent [in] - A callback function.
     */
    explicit profiler(onevent_t onevent = {});

    /**
     * Destructor.
     * @throw None.
     */
    ~profiler() noexcept;

    //!< Gets current duration.
    auto now() const noexcept -> duration_t;
  };
//-------------------------------------------------------------------------//
  auto get_duration_in_seconds_as_string(profiler::duration_t elapsed) -> std::string;
  auto get_duration_in_seconds_as_string(float elapsed) -> std::string;
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
#endif // __PROFILER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
