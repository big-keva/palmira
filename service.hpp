# if !defined( __palmira_service_hpp__ )
# define __palmira_service_hpp__
# include "DeliriX/DOM-text.hpp"
# include <mtc/interfaces.h>
# include <mtc/zmap.h>

namespace palmira {

  struct AccessArgs
  {
    std::string objectId;

    AccessArgs() = default;
    AccessArgs( const std::string& entId ): objectId( entId ) {}
    AccessArgs( const mtc::zmap& );
  };

  struct RemoveArgs: public AccessArgs
  {
    uint64_t    uVersion;
    mtc::zval   ifClause;

    RemoveArgs() = default;
    RemoveArgs(
      const std::string&  entId,
      uint64_t            uVers = 0,
      const mtc::zval&    icond = {} ): AccessArgs( entId ), uVersion( uVers ), ifClause( icond ) {}
    RemoveArgs( const mtc::zmap& );
  };

  struct UpdateArgs: RemoveArgs
  {
    mtc::zmap   metadata;

    UpdateArgs() = default;
    UpdateArgs(
      const std::string&  entId,
      const mtc::zmap&    mdata,
      uint64_t            uVers = 0,
      const mtc::zval&    icond = {} ): RemoveArgs( entId, uVers, icond ), metadata( mdata ) {}
    UpdateArgs( const mtc::zmap& );
  };

  class InsertArgs: public UpdateArgs
  {
    DeliriX::Text document;

  public:
    const DeliriX::ITextView& textview;

    InsertArgs(): textview( document ) {}
    InsertArgs(
      const std::string&        entId,
      const DeliriX::ITextView& tview,
      const mtc::zmap&          mdata = {},
      uint64_t                  uVers = 0,
      const mtc::zval&          icond = {} ): UpdateArgs( entId, mdata, uVers, icond ), textview( tview ) {}
    InsertArgs( const mtc::zmap& );

    auto  GetTextAPI() -> DeliriX::IText& {  return document;  }

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
    struct IPending;

    virtual auto  Insert( const InsertArgs& ) -> mtc::api<IPending> = 0;
    virtual auto  Update( const UpdateArgs& ) -> mtc::api<IPending> = 0;
    virtual auto  Remove( const RemoveArgs& ) -> mtc::api<IPending> = 0;
    virtual auto  Search( const SearchArgs& ) -> mtc::api<IPending> = 0;
    virtual void  Commit() = 0;
  };

  struct IService::IPending: Iface
  {
    virtual auto  Wait( double = -1.0 ) -> mtc::zmap = 0;
  };

}

# endif   // !__palmira_service_hpp__
