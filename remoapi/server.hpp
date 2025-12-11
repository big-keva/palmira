# if !defined( __palmira_remoapi_server_hpp__ )
# define __palmira_remoapi_server_hpp__
# include "../server.hpp"
# include "../service.hpp"

namespace remoapi
{
  auto  CreateServer( mtc::api<palmira::IService>, uint16_t port ) -> mtc::api<palmira::IServer>;
}

# endif // !__palmira_remoapi_server_hpp__
