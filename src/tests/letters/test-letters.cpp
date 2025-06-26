# include "texts/DOM-text.hpp"
# include "texts/DOM-sink.hpp"
# include <moonycode/codes.h>
# include <mtc/serialize.h>
# include <mtc/json.h>

int  main()
{
  auto  text = palmira::texts::Document<std::allocator<char>>( std::allocator<char>() );
    text.Attach();

  text.AddString( 0U, "aaa" );
  text.AddString( codepages::mbcstowide( codepages::codepage_utf8, "bbb" ) );

  {
    auto  body = text.AddTextTag( "body" );

      body->AddString( codepages::codepage_utf8, "first line in <body>" );
      body->AddString( codepages::mbcstowide( codepages::codepage_utf8, "second line in <body>" ) );

      auto  para = body->AddTextTag( "p" );

        para->AddString( codepages::codepage_utf8, "first line in <p> tag" );
        para->AddString( codepages::codepage_utf8, "second line in <p> tag" );

      body->AddString( codepages::codepage_utf8, "second line in <body>" );

      auto  span = body->AddTextTag( "span" );
        span->AddString( codepages::codepage_utf8, "text line in <span>" );
//      para->AddString( codepages::codepage_utf8, ""must cause exception" );

      body->AddString( codepages::codepage_utf8, "third line in <body>" );

    text.AddString( codepages::codepage_utf8, "last text line" );
  }

  auto  tagged = palmira::texts::dump_as::Json( palmira::texts::dump_as::MakeSink( stdout ) );
  text.Serialize( tagged.ptr() );
}

// tags implementation

namespace palmira {
namespace texts {
namespace dump_as {

  using FnSink = std::function<void(const char*, size_t)>;

  class TagsTag final: public IText
  {
    FnSink          fWrite;
    const unsigned  encode = codepages::codepage_utf8;
    unsigned        uShift;
    std::string     tagStr;

    implement_lifetime_control

  public:
    TagsTag( FnSink f, unsigned c, unsigned u, const std::string& t = {} ):
      fWrite( f ),
      encode( c ),
      uShift( u ),
      tagStr( t )
    {
      if ( !tagStr.empty() )
      {
        for ( unsigned u = 0; u != uShift; ++u )
          fWrite( "  ", 2 );

        fWrite( "<", 1 );
          Print( tagStr.c_str(), tagStr.length() );
        fWrite( ">\n", 2 );
      }
    }
    ~TagsTag()
    {
      if ( !tagStr.empty() )
      {
        for ( unsigned u = 0; u != uShift; ++u )
          fWrite( "  ", 2 );

        fWrite( "</", 2 );
          Print( tagStr.c_str(), tagStr.length() );
        fWrite( ">\n", 2 );
      }
    }
    auto  AddTextTag( const char* tag, size_t len ) -> mtc::api<IText> override
    {
      return new TagsTag( fWrite, encode, uShift + 1, std::string( tag, len ) );
    }
    void  AddCharStr( unsigned codepage, const char* str, size_t len ) override
    {
      std::string  sprint;

      if ( len == (size_t)-1 )
        for ( len = 0; str[len] != 0; ++len ) (void)NULL;

      if ( encode != unsigned(-1) && codepage != encode )
      {
        sprint = codepages::mbcstombcs( encode, codepage, str, len );
        str = sprint.c_str();
        len = sprint.size();
      }

      if ( !tagStr.empty() )
      {
        for ( unsigned u = 0; u <= uShift; ++u )
          fWrite( "  ", 2 );
      }

      Print( str, len );
      fWrite( "\n", 1 );
    }
    void  AddWideStr( const widechar* wcs, size_t len ) override
    {
      AddCharStr( encode, codepages::widetombcs( encode, wcs, len ).c_str(), -1 );
    }
  protected:
    void  Print( const char* str, size_t len ) const
    {
      auto end = str + len;

      while ( str != end )
      {
        auto org = str;

        while ( str != end && *str != '<' && *str != '>' && *str != '&' )
          ++str;

        if ( str != org ) fWrite( org, str - org );
          else
        switch ( *str++ )
        {
          case '<': fWrite( "&lt;", 4 );  break;
          case '>': fWrite( "&gt;", 4 );  break;
          default:  fWrite( "&amp;", 5 );
        }
      }
    }
  };

  class JsonTag final: public IText
  {
    FnSink      fWrite;
    unsigned    uShift;
    size_t      nItems = 0;

    implement_lifetime_control

  public:
    JsonTag( FnSink f, unsigned u, const std::string& t = {} ):
      fWrite( f ),
      uShift( u )
    {
      if ( !t.empty() )
      {
        for ( unsigned u = 0; u != uShift; ++u )
          fWrite( "  ", 2 );

        fWrite( "{ \"", 3 );
          Print( t.c_str(), t.length() );
        fWrite( "\": ", 3 );
      }
      fWrite( "[\n", 1 );
    }
    ~JsonTag()
    {
      if ( nItems != 0 )
        fWrite( "\n", 1 );
      for ( unsigned u = 0; u != uShift; ++u )
        fWrite( "  ", 2 );
      fWrite( "] }", 3 );
    }
    auto  AddTextTag( const char* tag, size_t len ) -> mtc::api<IText> override
    {
      if ( len == (size_t)-1 )
        for ( len = 0; tag[len] != 0; ++len ) (void)NULL;

      if ( nItems != 0 )  fWrite( ",\n", 2 );
        else fWrite( "\n", 1 );

      return new JsonTag( fWrite, uShift + 1, std::string( tag, len ) );
    }
    void  AddCharStr( unsigned codepage, const char* str, size_t len ) override
    {
      auto  putstr = std::string();

      if ( len == (size_t)-1 )
        for ( len = 0; str[len] != 0; ++len ) (void)NULL;

      if ( nItems != 0 )  fWrite( ",\n", 2 );
        else fWrite( "\n", 1 );

      for ( unsigned u = 0; u != uShift + 1; ++u )
        fWrite( "  ", 2 );

      if ( codepage != codepages::codepage_utf8 )
        putstr = codepages::mbcstombcs( codepages::codepage_utf8, codepage, str, len );
      else putstr = std::string( str, len );

      fWrite( "\"", 1 );
        Print( putstr.c_str(), putstr.length() );
      fWrite( "\"", 1 );

      ++nItems;
    }
    void  AddWideStr( const widechar* wcs, size_t len ) override
    {
      AddCharStr( codepages::codepage_utf8, codepages::widetombcs(
        codepages::codepage_utf8, wcs, len ).c_str(), -1 );
    }
  protected:
    void  Print( const char* str, size_t len ) const
    {
      auto end = str + len;

      while ( str != end )
      {
        auto org = str;

        while ( str != end && *str != '<' && *str != '>' && *str != '&' )
          ++str;

        if ( str != org ) fWrite( org, str - org );
          else
        switch ( *str++ )
        {
          case '<': fWrite( "&lt;", 4 );  break;
          case '>': fWrite( "&gt;", 4 );  break;
          default:  fWrite( "&amp;", 5 );
        }
      }
    }
  };

  auto  Tags( std::function<void( const char*, size_t )> fn ) -> mtc::api<IText>
  {
    return new TagsTag( fn, codepages::codepage_utf8, 0 );
  }

  auto  Json( std::function<void( const char*, size_t )> fn ) -> mtc::api<IText>
  {
    return new JsonTag( fn, 0 );
  }

}}}