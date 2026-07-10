#include "server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "server/http-server.h"
//-------------------------------------------------------------------------//
auto createServer(service_t service, const mtc::config &config) -> server_t {
  try {
    return new docapi::HttpServer(std::move(service), not config.empty() ? config.to_zmap() : mtc::zmap{});
  } catch (const std::exception &exc) {
    std::fprintf(stderr, "%s\n", exc.what());
  }
  return nullptr;
}

extern "C" auto getListenPort() -> std::uint16_t {
  return docapi::getListenPort();
}