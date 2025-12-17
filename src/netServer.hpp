# if !defined( __palmira_netServer_hpp__ )
# define __palmira_netServer_hpp__
# include "../server.hpp"
# include "../service.hpp"
# include <mtc/config.h>

namespace palmira
{
  auto  CreateGrpcServer( mtc::api<IService>, mtc::config& ) -> mtc::api<IServer>;
  auto  CreateHttpServer( mtc::api<IService>, mtc::config& ) -> mtc::api<IServer>;
  auto  CreateInetServer( mtc::api<IService>, mtc::config& ) -> mtc::api<IServer>;
}

# endif   // !__palmira_netServer_hpp__
