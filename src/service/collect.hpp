# if !defined( __palmira_src_service_collect_hpp__ )
# define __palmira_src_service_collect_hpp__
# include "structo/contents.hpp"
# include "structo/queries.hpp"
# include "structo/compat.hpp"
# include "structo/fields.hpp"
# include <mtc/threadPool.hpp>
# include <mtc/zmap.h>

namespace palmira {
namespace collect {

  using IQuery         = structo::queries::IQuery;
  using IContentsIndex = structo::IContentsIndex;
  using Abstract       = structo::queries::Abstract;

  struct ICollector: public mtc::Iface
  {
    virtual void  Search( mtc::api<IQuery> ) = 0;
    virtual auto  Finish( mtc::api<IContentsIndex> ) -> mtc::zmap = 0;
  };

  using DifferFn = std::function<int( uint32_t, double, uint32_t, double )>;
  using RankerFn = std::function<double( uint32_t, const Abstract& )>;
  using QuotesFn = std::function<mtc::array_zval( uint32_t, const Abstract& )>;

  class Documents final
  {
    class data;
    class impl;

    std::shared_ptr<data> params;

  public:
    Documents() = default;

    auto  SetFirst( uint32_t          nFirst ) -> Documents&;
    auto  SetCount( uint32_t          nCount ) -> Documents&;
    auto  SetOrder( DifferFn          fnComp ) -> Documents&;   // default by range
    auto  SetRange( RankerFn          ranker ) -> Documents&;
    auto  SetQuote( QuotesFn          quotes ) -> Documents&;
    auto  SetAsync( mtc::ThreadPool&  actors ) -> Documents&;

    auto  Create() -> mtc::api<ICollector>;
  };

  class LoadEntities final
  {
    class data;
    class impl;

    std::shared_ptr<data> params;

  public:
    LoadEntities( mtc::api<const IContentsIndex> );

    auto  Add( std::string_view, QuotesFn ) -> LoadEntities&;
    auto  Add( std::initializer_list<std::string_view>, QuotesFn ) -> LoadEntities&;
    auto  Add( std::initializer_list<std::pair<std::string_view, QuotesFn>> ) -> LoadEntities&;

    auto  Create() -> mtc::api<ICollector>;
  };

}}

# endif   // !__palmira_src_service_collect_hpp__
