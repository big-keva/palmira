# if !defined( __palmira_api_span_hxx__ )
# define __palmira_api_span_hxx__
# include <mtc/iBuffer.h>
# include <string_view>
# include <vector>
# include <atomic>

namespace palmira
{

  class Span
  {
    const char* ptr;
    size_t      len;

  public:
    auto  data() const -> const char* {  return ptr;  }
    auto  size() const -> size_t {  return len;  }

  public:
    Span(): ptr( nullptr ), len( 0 ) {}
    Span( const char* pch );
    Span( const char* pch, size_t cch ): ptr( pch ), len( cch ) {}
    Span( mtc::api<const mtc::IByteBuffer> );
    template <class Allocator>
    Span( const std::basic_string<char, std::char_traits<char>, Allocator>& src ):
      Span( src.data(), src.size() ) {}
    template <class Allocator>
    Span( const std::vector<char, Allocator>& src ):
      Span( src.data(), src.size() ) {}
  };

}

# endif   // __palmira_api_span_hxx__
