# if !defined( __palmira_service_z_arguments_hpp__ )
# define __palmira_service_z_arguments_hpp__
# include "DelphiX/textAPI/DOM-text.hpp"
# include "DelphiX/context/text-image.hpp"
# include <mtc/iBuffer.h>
# include <mtc/arena.hpp>
# include <mtc/zmap.h>

namespace palmira {

  struct RemoveArgs
  {
    std::string objectId;
    uint64_t    uVersion;
    mtc::zval   ifClause;

    RemoveArgs( const mtc::zmap& );
  };

  struct UpdateArgs: RemoveArgs
  {
    mtc::zmap   metadata;

    UpdateArgs( const mtc::zmap& );
  };

  class InsertArgs: public UpdateArgs
  {
    mtc::Arena  memArena;

    using Document = DelphiX::textAPI::BaseDocument<mtc::Arena::allocator<char>>;

    void  LoadBody( const mtc::zmap& );
  public:
    Document&   document;

    InsertArgs( const mtc::zmap& );
  };

}

# endif   // !__palmira_service_z_arguments_hpp__
