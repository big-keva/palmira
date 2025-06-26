# if !defined( __palmira_api_DOMini_hpp__ )
# define __palmira_api_DOMini_hpp__
# include <mtc/interfaces.h>
# include <mtc/wcsstr.h>
# include <string>

namespace palmira {
namespace letters {

  struct IText: public mtc::Iface
  {
    using charstr_view = std::basic_string_view<char>;
    using widestr_view = std::basic_string_view<widechar>;

    virtual auto  AddTextTag( const char*, size_t = -1 ) -> mtc::api<IText> = 0;
    virtual void  AddCharStr( unsigned, const char*, size_t = -1 ) = 0;
    virtual void  AddWideStr( const widechar*, size_t = -1 ) = 0;

  public:     // wrappers
    void  AddString( unsigned encoding, const char* str, size_t len = -1 )
      {  return AddCharStr( encoding, str, len );  }
    void  AddString( const widechar* str, size_t len = -1 )
      {  return AddWideStr( str, len );  }

    template <class Allocator>
    void  AddString( unsigned encoding, const std::basic_string<char, std::char_traits<char>, Allocator>& s )
      {  return AddString( encoding, s.c_str(), s.size() );  }
    template <class Allocator>
    void  AddString( const std::basic_string<widechar, std::char_traits<widechar>, Allocator>& s )
      {  return AddString( s.c_str(), s.size() );  }

  };

}}

# endif // !__palmira_api_DOMini_hpp__
