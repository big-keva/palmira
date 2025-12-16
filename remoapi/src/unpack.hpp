# if !defined( __palmira_remoapi_unpack_hpp__ )
# define __palmira_remoapi_unpack_hpp__
# include "../../service.hpp"
# include <remottp/request.hpp>
# include <mtc/iStream.h>

namespace remoapi
{
  auto  Inflate( const http::Message&, mtc::IByteStream* ) -> mtc::api<mtc::IByteStream>;
  auto  Deflate( const std::vector<char>& src ) -> std::vector<char>;

}

# endif   // !__palmira_remoapi_unpack_hpp__
