#include "../server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "http-server.h"
//-------------------------------------------------------------------------//
auto createServer(mtc::api<palmira::IService> service, const mtc::config &config) -> mtc::api<palmira::IServer>
{
  try
  {
    return new elastic::HttpServer(std::move(service), not config.empty() ? config.to_zmap() : mtc::zmap{});
  }
  catch (const std::exception &exc)
  {
    std::fprintf(stderr, "%s\n", exc.what());
  }
  return nullptr;
}

extern "C" auto getListenPort() -> std::uint16_t
{
  return elastic::getListenPort();
}