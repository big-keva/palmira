# if !defined( __palmira_service_hpp__ )
# define __palmira_service_hpp__
# include "DeliriX/DOM-text.hpp"
# include <mtc/interfaces.h>
# include <mtc/zmap.h>

namespace palmira {

  struct TimingArgs
  {
    double      fTimeout = -1.0;

    TimingArgs() = default;
    TimingArgs( const mtc::zmap& );
  };

  struct AccessArgs: TimingArgs
  {
    std::string objectId;

    AccessArgs() = default;
    AccessArgs( const std::string& entId ): objectId( entId ) {}
    AccessArgs( const mtc::zmap& );
  };

  struct RemoveArgs: AccessArgs
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
    auto  SetDocText( DeliriX::Text&& txt ) -> InsertArgs& {  return document = std::move( txt ), *this;  }

  private:
    void  LoadBody( const mtc::zmap& );

  };

  struct SearchArgs: TimingArgs
  {
    mtc::zval   query;
    mtc::zmap   order;
    mtc::zmap   terms;

    SearchArgs() = default;
    SearchArgs( const mtc::zval& req, const mtc::zmap& ord = {}, const mtc::zmap& tms = {} ):
      query( req ),
      order( ord ),
      terms( tms )  {}
  };

  struct IService: mtc::Iface
  {
    struct IPending;
    using  NotifyFn = std::function<void( const mtc::zmap& )>;

    virtual auto  Insert( const InsertArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> = 0;
    virtual auto  Update( const UpdateArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> = 0;
    virtual auto  Remove( const RemoveArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> = 0;
    virtual auto  Search( const SearchArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> = 0;
    virtual void  Commit() = 0;
  };

  struct IService::IPending: Iface
  {
    virtual auto  Wait( double = -1.0 ) -> mtc::zmap = 0;
  };

}

# endif   // !__palmira_service_hpp__
