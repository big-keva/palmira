# if !defined( __palmira_service_object_zmap_hpp__ )
# define __palmira_service_object_zmap_hpp__
# include "DeliriX/DOM-text.hpp"
# include <mtc/zmap.h>

namespace palmira {

  void  Serialize( mtc::zmap& to, const DeliriX::ITextView& );
  void  FetchFrom( const mtc::zmap& from, DeliriX::Text& doc );
  auto  ZmapAsText( mtc::array_zval& ) -> mtc::api<DeliriX::IText>;

}

# endif   // !__palmira_service_object_zmap_hpp__
