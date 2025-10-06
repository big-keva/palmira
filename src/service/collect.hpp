# if !defined( __palmira_src_service_collect_hpp__ )
# define __palmira_src_service_collect_hpp__
# include <DelphiX/text-api.hpp>
# include "DelphiX/contents.hpp"
# include "DelphiX/queries.hpp"
# include <mtc/zmap.h>

namespace palmira {
namespace collect {

  using IQuery         = DelphiX::queries::IQuery;
  using IContentsIndex = DelphiX::IContentsIndex;
  using ITextView      = DelphiX::textAPI::ITextView;
  using Abstract       = DelphiX::queries::Abstract;

  struct ICollector: public mtc::Iface
  {
    virtual void  Search( mtc::api<IQuery> ) = 0;
    virtual auto  Finish( mtc::api<IContentsIndex> ) -> mtc::zmap = 0;
  };

  struct IQuotation: public mtc::Iface
  {
    virtual auto  Create( uint32_t, const Abstract& ) -> mtc::api<ITextView> = 0;
  };

  class Documents final
  {
    class data;
    class impl;

    std::shared_ptr<data> params;

  public:
    Documents() = default;

    using IsLessFn = std::function<bool( uint32_t, double, uint32_t, double )>;
    using RankerFn = std::function<double( uint32_t, const Abstract& )>;

    auto  SetFirst( uint32_t ) -> Documents&;
    auto  SetCount( uint32_t ) -> Documents&;
    auto  SetOrder( IsLessFn ) -> Documents&;   // default by range
    auto  SetRange( RankerFn ) -> Documents&;
    auto  SetQuote( mtc::api<IQuotation> ) -> Documents&;

    auto  Create() -> mtc::api<ICollector>;
  };

}}

# endif   // !__palmira_src_service_collect_hpp__
