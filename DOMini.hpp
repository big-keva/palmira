# if !defined( __palmira_DOMini_hpp__ )
# define __palmira_DOMini_hpp__
#include <stdexcept>
# include <moonycode/codes.h>
# include <mtc/interfaces.h>
# include <string>

namespace palmira {

  struct MarkupTag
  {
    const char* format;
    uint32_t    uLower;     // start offset, bytes
    uint32_t    uUpper;     // end offset, bytes
  };

  struct TextChunk
  {
    unsigned    encode;
    size_t      length;
    const void* strptr;

  public:
    auto  GetCharStr() const -> std::basic_string_view<char>;
    auto  GetWideStr() const -> std::basic_string_view<widechar>;

  };

  struct IText: public mtc::Iface
  {
    virtual auto  AddTextTag( const char*, size_t = -1 ) -> mtc::api<IText> = 0;
    virtual void  AddCharStr( unsigned, const char*, size_t = -1 ) = 0;
    virtual void  AddWideStr( const widechar*, size_t = -1 ) = 0;

  protected:
    template <class A>
    using char_string = std::basic_string<char, std::char_traits<char>, A>;
    template <class A>
    using wide_string = std::basic_string<widechar, std::char_traits<widechar>, A>;

  public:     // wrappers
    auto  AddMarkup( const std::string_view& ) -> mtc::api<IText>;
    void  AddString( const std::string_view&, unsigned = codepages::codepage_utf8 );
    void  AddString( const std::basic_string_view<widechar>& );

    template <class Allocator>
    void  AddString( const char_string<Allocator>&, unsigned = codepages::codepage_utf8 );
    template <class Allocator>
    void  AddString( const wide_string<Allocator>& );

  };

  // TextChunk implementation

  inline auto TextChunk::GetCharStr() const -> std::basic_string_view<char>
  {
    if ( encode == unsigned(-1) )
      throw std::logic_error( "invalid encoding" );
    return { (const char*)strptr, length };
  }

  inline auto TextChunk::GetWideStr() const -> std::basic_string_view<widechar>
  {
    if ( encode != unsigned(-1) )
      throw std::logic_error( "invalid encoding" );
    return { (const widechar*)strptr, length };
  }

  // IText implementation

  inline  auto  IText::AddMarkup( const std::string_view& str ) -> mtc::api<IText>
  {
    return AddTextTag( str.data(), str.size() );
  }

  inline  void  IText::AddString( const std::string_view& str, unsigned encoding )
  {
    return AddCharStr( encoding, str.data(), str.length() );
  }

  inline  void  IText::AddString( const std::basic_string_view<widechar>& str )
  {
    return AddWideStr( str.data(), str.length() );
  }

  template <class Allocator>
  void  IText::AddString( const char_string<Allocator>& s, unsigned encoding )
  {
    return AddString( { s.c_str(), s.size() }, encoding );
  }

  template <class Allocator>
  void  IText::AddString( const wide_string<Allocator>& s )
  {
    return AddWideStr( s.c_str(), s.size() );
  }

}

# endif // !__palmira_DOMini_hpp__
