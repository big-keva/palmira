# if !defined( __palmira_network_grpc_client_hpp__ )
# define __palmira_network_grpc_client_hpp__
# include "../service.hpp"

namespace grpcapi
{
  auto  CreateClient( const char* addr ) -> mtc::api<palmira::IService>;

}

# endif // !__palmira_network_grpc_client_hpp__
