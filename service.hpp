# if !defined( __palmira_service_hpp__ )
# define __palmira_service_hpp__
# include <mtc/zmap.h>
# include <functional>

namespace palmira {

  using SearchService = std::function<mtc::zmap( const mtc::zmap& )>;

}

# endif   // !__palmira_service_hpp__
