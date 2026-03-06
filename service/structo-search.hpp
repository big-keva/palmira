# if !defined( __palmira_structo_search_hpp__ )
# define __palmira_structo_search_hpp__
# include "structo/context/processor.hpp"
# include "structo/context/x-contents.hpp"
# include "service.hpp"
# include <mtc/config.h>

namespace palmira {

  using namespace structo;

  using FnContents = std::function<context::Contents(
    const mtc::span<const mtc::span<const context::Lexeme>>&,
    const mtc::span<const DeliriX::MarkupTag>&,
    FieldHandler& )>;

  class StructoService
  {
    class data;

  public:
    auto  Set( FnContents )               -> StructoService&;
    auto  Set( mtc::api<IContentsIndex> ) -> StructoService&;
    auto  Set( context::Processor&& )     -> StructoService&;
    auto  Set( const context::Processor& ) -> StructoService&;
    auto  Set( const context::FieldManager& ) -> StructoService&;

  public:
    auto  Create() -> mtc::api<IService>;

  protected:
    std::shared_ptr<data> init;

  };

  auto  CreateStructo( const mtc::config& config ) -> mtc::api<IService>;

}

# endif   // !__palmira_structo_search_hpp__
