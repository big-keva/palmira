/*!==========================================================================
* \file
* - Program:       docapi-http
* - File:          server.h
* - Created:       06/23/2026
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
#ifndef __SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <mtc/config.h>
//-------------------------------------------------------------------------//
#include "server.hpp"
#include "service.hpp"
//-------------------------------------------------------------------------//
using server_t = mtc::api<palmira::IServer>;
using service_t = mtc::api<palmira::IService>;
//-------------------------------------------------------------------------//
/**
 * Creates a new server.
 * @param service [in] - A service.
 * @param config [in] - A server configuration.
 * @return A server instance.
 */
extern "C" auto createServer(service_t service, const mtc::config &config) -> server_t;

/**
 * Gets a listen port.
 * @return A listen port.
 */
extern "C" auto getListenPort() -> std::uint16_t;
//-------------------------------------------------------------------------//
#endif // __SERVER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
