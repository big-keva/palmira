# include "DelphiX/context/processor.hpp"
# include "service.hpp"

namespace palmira {

  using namespace DelphiX;

  using FnContents = std::function<mtc::api<IContents>(
    const Slice<const Slice<const context::Lexeme>>&,
    const Slice<const textAPI::MarkupTag>&, FieldHandler& )>;

  class DelphiXService
  {
    class data;

  public:
    auto  Set( FnContents ) -> DelphiXService&;
    auto  Set( mtc::api<IContentsIndex> ) -> DelphiXService&;
    auto  Set( context::Processor&& ) -> DelphiXService&;
    auto  Set( const context::Processor& ) -> DelphiXService&;

  public:
    auto  Create() -> mtc::api<IService>;

  protected:
    std::shared_ptr<data> init;

  };

}