# if !defined( __palmira_service_hpp__ )
# define __palmira_service_hpp__
# include "DelphiX/text-api.hpp"
# include <mtc/interfaces.h>
# include <mtc/arena.hpp>
# include <mtc/zmap.h>

namespace palmira {

  struct AccessArgs
  {
    std::string objectId;

    AccessArgs( const std::string& entId ): objectId( entId ) {}
    AccessArgs( const mtc::zmap& );
  };

  struct RemoveArgs: public AccessArgs
  {
    uint64_t    uVersion;
    mtc::zval   ifClause;

    RemoveArgs(
      const std::string&  entId,
      uint64_t            uVers = 0,
      const mtc::zval&    icond = {} ): AccessArgs( entId ), uVersion( uVers ), ifClause( icond ) {}
    RemoveArgs( const mtc::zmap& );
  };

  struct UpdateArgs: RemoveArgs
  {
    mtc::zmap   metadata;

    UpdateArgs(
      const std::string&  entId,
      const mtc::zmap&    mdata,
      uint64_t            uVers = 0,
      const mtc::zval&    icond = {} ): RemoveArgs( entId, uVers, icond ), metadata( mdata ) {}
    UpdateArgs( const mtc::zmap& );
  };

  class InsertArgs: public UpdateArgs
  {
    mtc::Arena  memArena;
    void*       document;

  public:
    const DelphiX::textAPI::ITextView&  textview;

    InsertArgs(
      const std::string&                  entId,
      const DelphiX::textAPI::ITextView&  tview,
      const mtc::zmap&                    mdata = {},
      uint64_t                            uVers = 0,
      const mtc::zval&                    icond = {} ): UpdateArgs( entId, mdata, uVers, icond ), textview( tview ) {}
    InsertArgs( const mtc::zmap& );

  private:
    void  LoadBody( const mtc::zmap& );

  };

  struct SearchArgs
  {
    mtc::zval   query;
    mtc::zmap   order;
    mtc::zmap   terms;
  };

  struct IService: mtc::Iface
  {
    virtual auto  Insert( const InsertArgs& ) -> mtc::zmap = 0;
    virtual auto  Update( const UpdateArgs& ) -> mtc::zmap = 0;
    virtual auto  Remove( const RemoveArgs& ) -> mtc::zmap = 0;
    virtual auto  Search( const SearchArgs& ) -> mtc::zmap = 0;
    virtual void  Commit() = 0;
  };

}

# endif   // !__palmira_service_hpp__
