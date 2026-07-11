# if !defined( __palmira_remoapi_loader_hpp__ )
# define __palmira_remoapi_loader_hpp__
# include "../../../service.hpp"
# include <remottp/request.hpp>
# include <mtc/iStream.h>

namespace remoapi
{
  namespace json
  {
    auto  Load( palmira::AccessArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::AccessArgs&;
    auto  Load( palmira::RemoveArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::RemoveArgs&;
    auto  Load( palmira::UpdateArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::UpdateArgs&;
    auto  Load( palmira::InsertArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::InsertArgs&;
    auto  Load( palmira::SearchArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::SearchArgs&;
  }
  namespace zmap
  {
    auto  Load( palmira::AccessArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::AccessArgs&;
    auto  Load( palmira::RemoveArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::RemoveArgs&;
    auto  Load( palmira::UpdateArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::UpdateArgs&;
    auto  Load( palmira::InsertArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::InsertArgs&;
    auto  Load( palmira::SearchArgs&, const http::Request&, mtc::IByteStream* ) -> palmira::SearchArgs&;
  }
}

# endif   // !__palmira_remoapi_loader_hpp__
