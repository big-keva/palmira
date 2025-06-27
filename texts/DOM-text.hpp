# if !defined( __palmira_DOM_text_hpp__ )
# define __palmira_DOM_text_hpp__
# include "DOMini.hpp"
# include <moonycode/codes.h>
# include <functional>
# include <stdexcept>
# include <vector>
# include <string>

namespace palmira {
namespace texts {

  template <class Allocator = std::allocator<char>>
  class BaseDocument final: public IText
  {
    class line;
    class span;

    template <class C>
    using basestr = std::basic_string<C, std::char_traits<C>, Allocator>;
    using charstr = basestr<char>;
    using widestr = basestr<widechar>;

    template <class To>
    using rebind = typename std::allocator_traits<Allocator>::template rebind_alloc<To>;

    class Markup;

    implement_lifetime_stub

  public:
    auto  AddTextTag( const char*, size_t = -1 ) -> mtc::api<IText> override;
    void  AddCharStr( unsigned, const char*, size_t = -1 ) override;
    void  AddWideStr( const widechar*, size_t = -1 ) override;

  public:
    BaseDocument( Allocator allocator = Allocator() ):
      lines( allocator ),
      spans( allocator )  {}

  public:
    void  clear();

    auto  GetLines() const -> const std::vector<line, Allocator>&
      {  return lines;  }
    auto  GetSpans() const -> const std::vector<span, Allocator>&
      {  return spans;  }

  public:
    auto  Serialize( IText* ) const -> IText*;

  protected:
    std::vector<line, Allocator>  lines;
    std::vector<span, Allocator>  spans;
    mtc::api<Markup>              pspan;
    uint32_t                      chars = 0;

  };

  using Document = BaseDocument<>;

  template <class Allocator>
  class BaseDocument<Allocator>::line
  {
    using strspace = typename std::aligned_storage<std::max(sizeof(charstr), sizeof(widestr)),
      alignof(charstr)>::type;

  public:
    line( Allocator alloc, unsigned encode, const char* str, size_t length );
    line( Allocator alloc, const widechar* str, size_t length );
    line( const line& );
    line( line&& );
   ~line();

  public:
    auto  GetCharStr() const -> const charstr&;
    auto  GetWideStr() const -> const widestr&;
    auto  GetCharStr() -> charstr&;
    auto  GetWideStr() -> widestr&;
    auto  GetTextLen() const -> uint32_t;

    uint32_t  encode;

  protected:
    strspace  string;

  };

  template <class Allocator>
  class BaseDocument<Allocator>::span
  {
  public:
    span( uint32_t l, uint32_t u, charstr&& s ):
      uLower( l ),
      uUpper( u ),
      tagStr( std::move( s ) )  {}

  public:
    uint32_t  uLower;     // start offset, bytes
    uint32_t  uUpper;     // end offset, bytes
    charstr   tagStr;
  };

  template <class Allocator>
  class BaseDocument<Allocator>::Markup final: public IText
  {
    friend class BaseDocument<Allocator>;

    std::atomic_long  refCount = 0;

  public:
    long  Attach() override {  return ++refCount;  }
    long  Detach() override;

  protected:
    auto  AddTextTag( const char*, size_t ) -> mtc::api<IText> override;
    void  AddCharStr( unsigned, const char*, size_t ) override;
    void  AddWideStr( const widechar*, size_t ) override;

    auto  LastMarkup( Markup* ) -> Markup*;
    void  Close();

  public:
    Markup( BaseDocument*, const char* tag, size_t length );
    Markup( Markup*, const char* tag, size_t length );
   ~Markup();

  protected:
    mtc::api<BaseDocument>  docptr;
    mtc::api<Markup>        nested;
    size_t                  tagBeg;

  };

  // Document implementation

  template <class Allocator>
  auto  BaseDocument<Allocator>::AddTextTag( const char* tag, size_t len ) -> mtc::api<IText>
  {
    if ( len == size_t(-1) )
      for ( ++len; tag[len] != 0; ++len ) (void)NULL;

    if ( pspan != nullptr )
      pspan->Close();

    return (pspan = new( rebind<Markup>( spans.get_allocator() ).allocate( 1 ) )
      Markup( this, tag, len )).ptr();
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::AddCharStr( unsigned codepage, const char* str, size_t len )
  {
    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( pspan != nullptr )
      pspan->Close();

    lines.emplace_back( lines.get_allocator(), codepage, str, len );
      chars += len;
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::AddWideStr( const widechar* str, size_t len )
  {
    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( pspan != nullptr )
      pspan->Close();

    lines.emplace_back( lines.get_allocator(), str, len );
      chars += len;
  }

  template <class Allocator>
  void    BaseDocument<Allocator>::clear()
  {
    if ( pspan != nullptr )
      {  pspan->Close();  pspan = nullptr;  }
    lines.clear();
    spans.clear();
    chars = 0;
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::Serialize( IText* text ) const -> IText*
  {
    auto  lineIt = lines.begin();
    auto  spanIt = spans.begin();
    auto  offset = uint32_t(0);
    auto  fPrint = std::function<void( IText*, uint32_t )>( [&]( IText* to, uint32_t up )
      {
        while ( lineIt != lines.end() && offset < up )
        {
        // check if print next line to current IText*
          if ( spanIt == spans.end() || offset < spanIt->uLower )
          {
            if ( lineIt->encode == 0x80000000 ) to->AddString( lineIt->GetWideStr() );
              else
            if ( lineIt->encode != 0xffffffff ) to->AddString( lineIt->encode, lineIt->GetCharStr() );

            offset += lineIt->GetTextLen();

            if ( ++lineIt == lines.end() )  return;
              continue;
          }

        // check if open new span
          if ( offset >= spanIt->uLower )
          {
            auto  new_to = to->AddTextTag( spanIt->tagStr.c_str(), spanIt->tagStr.length() );
            auto  uUpper = spanIt->uUpper;
              ++spanIt;
            fPrint( new_to.ptr(), uUpper );
          }
        }
      } );

    fPrint( text, chars );

    return text;
  }

  // Document::line template implementation

  template <class Allocator>
  BaseDocument<Allocator>::line::line( Allocator alloc, unsigned encoding, const char* str, size_t len )
  {
    if ( encoding == 0x80000000 )
      throw std::invalid_argument( "invalid encoding" );

    new( &string ) charstr( str, len, alloc );
      encode = encoding;
  }

  template <class Allocator>
  BaseDocument<Allocator>::line::line( Allocator alloc, const widechar* str, size_t len )
  {
    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;
    new( &string ) widestr( str, len, alloc );
      encode = 0x80000000;
  }

  template <class Allocator>
  BaseDocument<Allocator>::line::line( const line& l ): encode( l.encode )
  {
    if ( encode == 0x80000000 ) new( &string ) widestr( *(const widestr*)&l.string );
      else
    if ( encode != 0xffffffff ) new( &string ) charstr( *(const charstr*)&l.string );
  }

  template <class Allocator>
  BaseDocument<Allocator>::line::line( line&& l ): encode( l.encode )
  {
    if ( encode == 0x80000000 ) new( &string ) widestr( std::move( *(widestr*)&l.string ) );
      else
    if ( encode != 0xffffffff ) new( &string ) charstr( std::move( *(charstr*)&l.string ) );
  }

  template <class Allocator>
  BaseDocument<Allocator>::line::~line()
  {
    if ( encode == 0x80000000 ) ((widestr*)&string)->~widestr();
      else
    if ( encode != 0xffffffff ) ((charstr*)&string)->~charstr();
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::line::GetCharStr() const -> const charstr&
  {
    if ( encode == 0x80000000 )
      throw std::logic_error( "invalid encoding" );
    return *((const charstr*)&string);
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::line::GetCharStr() -> charstr&
  {
    if ( encode == 0x80000000 )
      throw std::logic_error( "invalid encoding" );
    return *((charstr*)&string);
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::line::GetWideStr() const -> const widestr&
  {
    if ( encode != 0x80000000 )
      throw std::logic_error( "invalid encoding" );
    return *((const widestr*)&string);
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::line::GetWideStr() -> widestr&
  {
    if ( encode != 0x80000000 )
      throw std::logic_error( "invalid encoding" );
    return *((widestr*)&string);
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::line::GetTextLen() const -> uint32_t
  {
    if ( encode == 0x80000000 )
      return ((widestr*)&string)->length();

    if ( encode != 0xffffffff )
      return ((charstr*)&string)->length();

    throw std::logic_error( "invalid encoding" );
  }

  // Document::Markup template implementation

  template <class Allocator>
  BaseDocument<Allocator>::Markup::Markup( BaseDocument* owner, const char* tag, size_t len ):
    docptr( owner ),
    tagBeg( owner->spans.size() )
  {
    auto  tagstr = charstr( tag, len, owner->spans.get_allocator() );

    owner->spans.emplace_back( uint32_t(owner->chars), uint32_t(-1),
      std::move( tagstr ) );
  }

  template <class Allocator>
  BaseDocument<Allocator>::Markup::Markup( Markup* owner, const char* tag, size_t len ):
    docptr( owner->docptr ),
    tagBeg( docptr->spans.size() )
  {
    auto  tagstr = charstr( tag, len, docptr->spans.get_allocator() );

    docptr->spans.emplace_back( uint32_t(docptr->chars), uint32_t(-1),
      std::move( tagstr ) );
  }

  template <class Allocator>
  BaseDocument<Allocator>::Markup::~Markup()
  {
    Close();
  }

  template <class Allocator>
  long  BaseDocument<Allocator>::Markup::Detach()
  {
    auto  rcount = --refCount;

    if ( rcount == 0 )
    {
      auto  memman = rebind<Markup>( docptr->lines.get_allocator() );

      this->~Markup();
      memman.deallocate( this, 0 );
    }
    return rcount;
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::Markup::AddTextTag( const char* tag, size_t len ) -> mtc::api<IText>
  {
    Markup* last;

    if ( len == size_t(-1) )
      for ( ++len; tag[len] != 0; ++len ) (void)NULL;

    if ( (last = LastMarkup( this )) != nullptr )
      last->Close();

    return (nested = new( rebind<Markup>( docptr->spans.get_allocator() ).allocate( 1 ) )
      Markup( this, tag, len )).ptr();
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::Markup::AddCharStr( unsigned encoding, const char* str, size_t len )
  {
    Markup* last;

    if ( tagBeg == size_t(-1) )
      throw std::logic_error( "attempt of adding line to closed markup" );

    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( (last = LastMarkup( nullptr )) != this )
      last->Close();

    docptr->lines.emplace_back( docptr->lines.get_allocator(),
      encoding, str, len );
    docptr->chars += len;
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::Markup::AddWideStr( const widechar* str, size_t len )
  {
    Markup*  last;

    if ( tagBeg == size_t(-1) )
      throw std::logic_error( "attempt of adding line to closed markup" );

    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( (last = LastMarkup( this )) != nullptr )
      last->Close();

    docptr->lines.emplace_back( docptr->lines.get_allocator(),
      str, len );
    docptr->chars += len;
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::Markup::LastMarkup( Markup* onPath ) -> Markup*
  {
    auto  markup = docptr->pspan;

    while ( markup != nullptr )
    {
      if ( markup == onPath )
        return nullptr;
      if ( markup->nested != nullptr )  markup = markup->nested;
        else return markup;
    }

    return markup;
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::Markup::Close()
  {
    if ( nested != nullptr )
      {  nested->Close();  nested = nullptr;  }

    if ( tagBeg != size_t(-1) )
    {
      docptr->spans[tagBeg].uUpper = docptr->chars - 1;
        tagBeg = size_t(-1);
    }
  }

}}

# endif // !__palmira_DOM_text_hpp__
