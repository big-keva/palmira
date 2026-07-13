#include "../server.h"
//-------------------------------------------------------------------------//
#include <utility>
//-------------------------------------------------------------------------//
#include "http-server.h"
//-------------------------------------------------------------------------//
auto CreateServer(palmira::IServer **server, palmira::IService *service, const mtc::config &config) -> int
{
  try
  {
    if (*server != nullptr)
    {
      // Releasing allocated resources.
      (*server)->Detach();
    }
    *server = new elastic::HttpServer(service, config.to_zmap());

    // Incrementing a reference.
    (*server)->Attach();
  }
  catch (const std::exception &exc)
  {
    return std::fprintf(stderr, "%s\n", exc.what()), -EINVAL;
  }
  return 0;
}

extern "C" auto getListenPort() -> std::uint16_t
{
  return elastic::getListenPort();
}