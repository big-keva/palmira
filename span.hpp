# if !defined( __palmira_span_hpp__ )
# define __palmira_span_hpp__
# include <mtc/iBuffer.h>

namespace palmira
{

  template <class T>
  class Slice
  {
  public:
    Slice():
      items( nullptr ),
      count( 0 )  {}
    Slice( const T* data, size_t size ):
      items( data ),
      count( size ) {}
    Slice( const Slice& ) = default;
  template <class Iterable>
    Slice( const Iterable& coll ):
      items( coll.data() ),
      count( coll.size() )  {}

  public:
    auto  begin() const {  return items;  }
    auto  end() const   {  return items + count;  }
    auto  data() const -> const T* {  return items;  }
    auto  size() const -> size_t   {  return count;  }
    bool  empty() const {  return count == 0;  }

    auto  at( size_t pos ) const -> const T& {  return items[pos];  }
    auto  operator[]( size_t pos ) const -> const T& {  return items[pos];  }

  protected:
    const T*  items;
    size_t    count;

  };

  class Span: public Slice<const char>
  {
    using Slice::Slice;

  public:
    Span( const char* pch );
    Span( mtc::api<const mtc::IByteBuffer> );
    template <class Collector>
    Span( const Collector& src ):
      Span( src.data(), src.size() ) {}
  };

}

# endif   // __palmira_span_hpp__
