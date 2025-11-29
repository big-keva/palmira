# if !defined( __palmira_src_service_collect_hpp__ )
# define __palmira_src_service_collect_hpp__
//# include "structo/text-api.hpp"
# include "structo/contents.hpp"
# include "structo/queries.hpp"
# include "structo/compat.hpp"
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

  class Documents final
  {
    class data;
    class impl;

    std::shared_ptr<data> params;

  public:
    Documents() = default;

    using IsLessFn = std::function<bool  ( uint32_t, double, uint32_t, double )>;
    using RankerFn = std::function<double( uint32_t, const Abstract& )>;
    using QuotesFn = std::function<mtc::array_zval( uint32_t, const Abstract& )>;

    auto  SetFirst( uint32_t ) -> Documents&;
    auto  SetCount( uint32_t ) -> Documents&;
    auto  SetOrder( IsLessFn ) -> Documents&;   // default by range
    auto  SetRange( RankerFn ) -> Documents&;
    auto  SetQuote( QuotesFn ) -> Documents&;

    auto  Create() -> mtc::api<ICollector>;
  };

}}

# endif   // !__palmira_src_service_collect_hpp__
