# if !defined( __palmira_grpcttp_client_hpp__ )
# define __palmira_grpcttp_client_hpp__
# include "../service.hpp"

namespace grpcttp
{
  auto  CreateClient( uint16_t port ) -> mtc::api<palmira::IService>;

}

# endif // !__palmira_grpcttp_client_hpp__
