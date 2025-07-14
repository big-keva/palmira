# include "contents-index.hpp"

namespace palmira {

  class Lister: public IContentsIndex::IIndexAPI
  {
    std::function<void( const Span&, const Span&, unsigned )> forward;

  public:
    Lister( std::function<void( const Span&, const Span&, unsigned )> fn ):
      forward( fn ) {}

  public:
    void  Insert( const Span& key, const Span& block, unsigned bkType ) override
    {
      forward( key, block, bkType );
    }

    auto  ptr() -> IIndexAPI* {  return static_cast<IIndexAPI*>( this );  }

  };

  // Span implementation

  Span::Span( const char* pch )
  {
    for ( items = pch, count = 0; pch[count] != 0; ++count  )
      (void)NULL;
  }

  Span::Span( mtc::api<const mtc::IByteBuffer> buf ):
    Span( buf != nullptr ? buf->GetPtr() : nullptr, buf != nullptr ? buf->GetLen() : 0 )
  {
  }

  // IContents implementation

  void  IContents::List( std::function<void( const Span&, const Span&, unsigned )> fn )
  {
    Enum( Lister( fn ).ptr() );
  }

}
