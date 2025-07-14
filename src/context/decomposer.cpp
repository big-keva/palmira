# include "context/decomposer.hpp"
# include <mtc/sharedLibrary.hpp>
# include <memory>

namespace palmira {
namespace query {

  class ExternalDecomposer
  {
    struct impl
    {
      mtc::SharedLibrary  module;
      DecomposeHolder*    holder = nullptr;
      DecomposeText       textFn = nullptr;
      DecomposeFree       freeFn = nullptr;

     ~impl()
      {
        if ( holder != nullptr && freeFn != nullptr )
          freeFn( holder );
      }
    };

    std::shared_ptr<impl> holder;

  public:
    ExternalDecomposer( mtc::SharedLibrary, const char* );

    bool  operator == ( const ExternalDecomposer& xp ) const  {  return holder == xp.holder;  }
    bool  operator == ( nullptr_t ) const {  return holder == nullptr;  }
    template <class T>
      bool  operator != ( T t ) const {  return !(*this == t);  }

  public:
    void  operator()( IImage*, const Slice<TextToken>&, const Slice<MarkupTag>& );
  };

  // DefaultDecomposer implementation

  void  DefaultDecomposer::operator()(
    IImage*                       image,
    const Slice<TextToken>&  words,
    const Slice<MarkupTag>&/*mtags*/)
  {
    for ( size_t pos = 0; pos != words.size(); ++pos )
    {
      auto& next = words[pos];
      auto  stem = codepages::strtolower( next.pwsstr, next.length );

      image->AddStem( pos, 0xff, stem.c_str(), stem.length(), 0, nullptr, 0 );
    }
  }

  // ExternalDecomposer implementation

  ExternalDecomposer::ExternalDecomposer( mtc::SharedLibrary mod, const char* args )
  {
    auto  fnInit = (DecomposeInit)mod.Find( "DecomposeInit" );
    auto  decomp = (DecomposeHolder*)nullptr;
    int   nerror;

    if ( (nerror = fnInit( &decomp, args )) != 0 )
      throw std::runtime_error( "Failed to initialize decomposer, error code: " + std::to_string( nerror ) );

    holder = std::make_shared<impl>();

    holder->module = mod;
    holder->holder = decomp;
    holder->textFn = (DecomposeText)mod.Find( "DecomposeText" );
    holder->freeFn = (DecomposeFree)mod.Find( "DecomposeFree" );
  }

}}

