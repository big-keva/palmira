# if !defined( __palmira_toolset_hpp__ )
# define __palmira_toolset_hpp__
# include "service.hpp"

namespace palmira
{

  auto  Quick( const mtc::zmap& ) -> mtc::api<IService::IPending>;

}

# endif // !__palmira_toolset_hpp__
