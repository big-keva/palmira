#pragma once
//-------------------------------------------------------------------------//
#include <mtc/config.h>
//-------------------------------------------------------------------------//
#include "server.hpp"
#include "service.hpp"
//-------------------------------------------------------------------------//
/**
 * Creates a new server.
 * @param server [out] - A created server.
 * @param service [in] - A service.
 * @param config [in] - A server configuration.
 * @return A server instance.
 */
extern "C" auto CreateServer(palmira::IServer **server, palmira::IService *service, const mtc::config &config) -> int;
