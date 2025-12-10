# if !defined( __palmira_grpcttp_server_hpp__ )
# define __palmira_grpcttp_server_hpp__
# include "../server.hpp"
# include "../service.hpp"

namespace grpcttp
{
  auto  CreateServer( mtc::api<palmira::IService>, uint16_t port ) -> mtc::api<palmira::IServer>;
}

# endif // !__palmira_grpcttp_server_hpp__
