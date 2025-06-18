# if !defined( __palmira_api_index_layered_contents_hxx__ )
# define __palmira_api_index_layered_contents_hxx__
# include "contents-index.hxx"
# include "dynamic-contents.hxx"
# include <functional>

namespace palmira {
namespace index {
namespace layered {

  class Contents
  {
    mtc::api<IStorage>    contentsStorage;
    dynamic::Settings     dynamicSettings;
    std::chrono::seconds  runMonitorDelay = std::chrono::seconds( 0 );

  public:
    auto  Set( const dynamic::Settings& ) -> Contents&;
    auto  Set( mtc::api<IStorage> ) -> Contents&;
    auto  Create() -> mtc::api<IContentsIndex>;

    static
    auto  Create( const mtc::api<IContentsIndex>*, size_t ) -> mtc::api<IContentsIndex>;
    static
    auto  Create( const std::vector<mtc::api<IContentsIndex>>& ) -> mtc::api<IContentsIndex>;

  };

}}}

# endif   // !__palmira_api_index_layered_contents_hxx__
