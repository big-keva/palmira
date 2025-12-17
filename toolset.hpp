# if !defined( __palmira_toolset_hpp__ )
# define __palmira_toolset_hpp__
# include "service.hpp"

namespace palmira
{
  auto  Immediate( const mtc::zmap&, IService::NotifyFn = {} ) -> mtc::api<IService::IPending>;
}

# endif // !__palmira_toolset_hpp__
