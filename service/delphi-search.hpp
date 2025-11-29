# if !defined( __palmira_delphi_search_hpp__ )
# define __palmira_delphi_search_hpp__
# include "structo/context/processor.hpp"
# include "service.hpp"

namespace palmira {

  using namespace structo;

  using FnContents = std::function<mtc::api<IContents>(
    const mtc::span<const mtc::span<const context::Lexeme>>&,
    const mtc::span<const DeliriX::MarkupTag>&, FieldHandler& )>;

  class StructoService
  {
    class data;

  public:
    auto  Set( FnContents )               -> StructoService&;
    auto  Set( mtc::api<IContentsIndex> ) -> StructoService&;
    auto  Set( context::Processor&& )     -> StructoService&;
    auto  Set( const context::Processor& ) -> StructoService&;

  public:
    auto  Create() -> mtc::api<IService>;

  protected:
    std::shared_ptr<data> init;

  };

}

# endif   // !__palmira_delphi_search_hpp__
