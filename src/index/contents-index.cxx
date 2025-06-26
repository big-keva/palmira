# include "span.hpp"

namespace palmira {

  // Span implementation

  Span::Span( const char* pch ): ptr( pch ), len( 0 )
  {
    while ( ptr[len] != 0 ) ++len;
  }

  Span::Span( mtc::api<const mtc::IByteBuffer> buf ):
    ptr( buf != nullptr ? buf->GetPtr() : nullptr ),
    len( buf != nullptr ? buf->GetLen() : 0 )  {}

}
