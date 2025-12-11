# if !defined( __palmira_remoapi_unpack_hpp__ )
# define __palmira_remoapi_unpack_hpp__
# include "../../service.hpp"
# include <remottp/request.hpp>
# include <mtc/iStream.h>

namespace remoapi
{
  auto  Unpack( const http::Request&, mtc::IByteStream* ) -> mtc::api<mtc::IByteStream>;
}

# endif   // !__palmira_remoapi_unpack_hpp__
