# if !defined( __palmira_src_index_merger_contents_hxx__ )
# define __palmira_src_index_merger_contents_hxx__
# include "contents-index.hpp"
# include "notify-events.hxx"

namespace palmira {
namespace index   {
namespace fusion {

  class Contents
  {
    std::vector<mtc::api<IContentsIndex>> indexVector;
    Notify::Func                          notifyEvent;
    std::function<bool()>                 canContinue;
    mtc::api<IStorage::IIndexStore>       outputStore;

  public:
    auto  Add( const mtc::api<IContentsIndex> ) -> Contents&;

    auto  Set( Notify::Func ) -> Contents&;
    auto  Set( std::function<bool()> ) -> Contents&;
    auto  Set( mtc::api<IStorage::IIndexStore> ) -> Contents&;
    auto  Set( const mtc::api<IContentsIndex>*, size_t ) -> Contents&;
    auto  Set( const std::vector<mtc::api<IContentsIndex>>& ) -> Contents&;
    auto  Set( const std::initializer_list<mtc::api<IContentsIndex>>& ) -> Contents&;

    auto  Create() -> mtc::api<IContentsIndex>;
  };

}}}

# endif // !__palmira_src_index_merger_contents_hxx__
