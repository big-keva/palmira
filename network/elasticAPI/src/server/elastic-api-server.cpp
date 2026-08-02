#include "../elastic-api-server.h"
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
    *server = new elastic::http_server(service, config.to_zmap());

    // Incrementing a reference.
    (*server)->Attach();
  }
  catch (const std::exception &exc)
  {
    return std::fprintf(stderr, "%s\n", exc.what()), -EINVAL;
  }
  return 0;
}
