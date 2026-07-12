#pragma once

#include <mtc/config.h>

#include "../server.hpp"
#include "../service.hpp"

namespace elasticapi
{
//-------------------------------------------------------------------------//
  auto CreateServer(mtc::api<palmira::IService>, const mtc::config &) -> mtc::api<palmira::IServer>;
//-------------------------------------------------------------------------//
} // namespace elasticapi
