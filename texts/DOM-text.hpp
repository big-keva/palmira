# if !defined( __palmira_DOM_text_hpp__ )
# define __palmira_DOM_text_hpp__
# include "DOMini.hpp"
# include <moonycode/codes.h>
# include <mtc/wcsstr.h>
# include <functional>
# include <stdexcept>
# include <vector>
# include <string>

namespace palmira {
namespace texts {

  template <class Allocator>
  class BaseDocument final: public IText
  {
    class  Str;

    template <class To>
    using rebind = typename std::allocator_traits<Allocator>::template rebind_alloc<To>;

    template <class C>
    using basestr = std::basic_string<C, std::char_traits<C>, rebind<C>>;
    using charstr = basestr<char>;
    using widestr = basestr<widechar>;

    class Markup;
    class UtfTxt;

    implement_lifetime_stub

  protected:
    struct Item
    {
      friend class BaseDocument;

      Item( const char* s ):
        str( s ),
        tag( nullptr ),
        arr( nullptr )  {}
      Item( const char* t, const std::initializer_list<Item>& l ):
        str( nullptr ),
        tag( t ),
        arr( &l ) {}

    protected:
      const char*                         str;
      const char*                         tag;
      const std::initializer_list<Item>*  arr;
    };

  public:
    auto  AddTextTag( const char*, size_t = -1 ) -> mtc::api<IText> override;
    void  AddCharStr( unsigned, const char*, size_t = -1 ) override;
    void  AddWideStr( const widechar*, size_t = -1 ) override;

  public:
    BaseDocument( Allocator allocator = Allocator() ):
      lines( allocator ),
      spans( allocator )  {}
    BaseDocument( const std::initializer_list<Item>&, Allocator = Allocator() );
   ~BaseDocument();

  public:
    void  clear();

    auto  GetBlocks() const -> const std::vector<TextChunk, Allocator>&
      {  return lines;  }
    auto  GetMarkup() const -> const std::vector<MarkupTag, Allocator>&
      {  return spans;  }

  public:
    auto  CopyUtf16( IText*, unsigned cp = codepages::codepage_utf8 ) const -> IText*;
    auto  Serialize( IText* ) const -> IText*;

  protected:
    std::vector<TextChunk, Allocator> lines;
    std::vector<MarkupTag, Allocator> spans;
    mtc::api<Markup>                  pspan;
    uint32_t                          chars = 0;

  };

  using Document = BaseDocument<std::allocator<char>>;

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

  template <class Allocator>
  class BaseDocument<Allocator>::UtfTxt final: public IText
  {
    implement_lifetime_control

  public:
    UtfTxt( mtc::api<IText> tx, unsigned cp ):
      output( tx ),
      encode( cp )  {}

    auto  AddTextTag( const char* tag, size_t len ) -> mtc::api<IText> override
      {  return new UtfTxt( output->AddTextTag( tag, len ), encode );  }
    void  AddCharStr( unsigned enc, const char* str, size_t len ) override
      {  output->AddString( codepages::mbcstowide( enc != 0 ? enc : encode, str, len ) );  }
    void  AddWideStr( const widechar* str, size_t len ) override
      {  return output->AddWideStr( str, len );  }

  protected:
    mtc::api<IText> output;
    unsigned        encode;

  };

  // Document implementation

  template <class Allocator>
  BaseDocument<Allocator>::BaseDocument( const std::initializer_list<Item>& init, Allocator mman ):
    lines( mman ),
    spans( mman )
  {
    auto  fnFill = std::function<void( mtc::api<IText>, const std::initializer_list<Item>& )>();

    fnFill = [&]( mtc::api<IText> to, const std::initializer_list<Item>& it )
      {
        for ( auto& next: it )
          if ( next.str != nullptr )  to->AddCharStr( codepages::codepage_utf8, next.str );
            else  fnFill( to->AddMarkup( next.tag ), *next.arr );
      };

    fnFill( this, init );
  }

  template <class Allocator>
  BaseDocument<Allocator>::~BaseDocument()
  {
    clear();
  }

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
    char* pstr;

    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( pspan != nullptr )
      pspan->Close();

    mtc::w_strncpy( pstr = rebind<char>( lines.get_allocator() ).allocate( len + 1 ),
      str, len )[len] = 0;

    lines.push_back( { codepage, len, (const void*)pstr } );
      chars += len;
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::AddWideStr( const widechar* str, size_t len )
  {
    widechar* pstr;

    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( pspan != nullptr )
      pspan->Close();

    mtc::w_strncpy( pstr = rebind<widechar>( lines.get_allocator() ).allocate( len + 1 ),
      str, len )[len] = 0;

    lines.push_back( { unsigned(-1), len, (const void*)pstr } );
      chars += len;
  }

  template <class Allocator>
  void    BaseDocument<Allocator>::clear()
  {
    if ( pspan != nullptr )
      {  pspan->Close();  pspan = nullptr;  }

    for ( auto& next: lines )
    {
      if ( next.encode == unsigned(-1) )
        rebind<widechar>( lines.get_allocator() ).deallocate( (widechar*)next.strptr, 0 );
      else
        rebind<char>( lines.get_allocator() ).deallocate( (char*)next.strptr, 0 );
    }

    for ( auto& next: spans )
      rebind<char>( spans.get_allocator() ).deallocate( (char*)next.format, 0 );

    lines.clear();
    spans.clear();
    chars = 0;
  }

  template <class Allocator>
  auto  BaseDocument<Allocator>::CopyUtf16( IText* output, unsigned encode ) const -> IText*
  {
    UtfTxt  utfOut( output, encode );
      Serialize( &utfOut );
    return output;
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
            if ( lineIt->encode == unsigned(-1) )
              to->AddString( lineIt->GetWideStr() );
            else
              to->AddString( lineIt->GetCharStr(), lineIt->encode );

            offset += lineIt->length;

            if ( ++lineIt == lines.end() )  return;
              continue;
          }

        // check if open new span
          if ( offset >= spanIt->uLower )
          {
            auto  new_to = to->AddTextTag( spanIt->format );
            auto  uUpper = spanIt->uUpper;
              ++spanIt;
            fPrint( new_to.ptr(), uUpper );
          }
        }
      } );

    fPrint( text, chars );

    return text;
  }

  // Document::Markup template implementation

  template <class Allocator>
  BaseDocument<Allocator>::Markup::Markup( BaseDocument* owner, const char* tag, size_t len ):
    docptr( owner ),
    tagBeg( owner->spans.size() )
  {
    auto  tagstr = strcpy( rebind<char>( owner->spans.get_allocator() )
      .allocate( len + 1 ), tag );

    owner->spans.push_back( { tagstr, uint32_t(owner->chars), uint32_t(-1) } );
  }

  template <class Allocator>
  BaseDocument<Allocator>::Markup::Markup( Markup* owner, const char* tag, size_t len ):
    docptr( owner->docptr ),
    tagBeg( docptr->spans.size() )
  {
    auto  tagstr = strcpy( rebind<char>( docptr->spans.get_allocator() )
      .allocate( len + 1 ), tag );

    docptr->spans.push_back( { tagstr, uint32_t(docptr->chars), uint32_t(-1) } );
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
    char*   pstr;

    if ( tagBeg == size_t(-1) )
      throw std::logic_error( "attempt of adding line to closed markup" );

    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( (last = LastMarkup( nullptr )) != this )
      last->Close();

    mtc::w_strncpy( pstr = rebind<char>( docptr->lines.get_allocator() ).allocate( len + 1 ),
      str, len )[len] = 0;

    docptr->lines.push_back( { encoding, len, pstr } );
      docptr->chars += len;
  }

  template <class Allocator>
  void  BaseDocument<Allocator>::Markup::AddWideStr( const widechar* str, size_t len )
  {
    Markup*   last;
    widechar* pstr;

    if ( tagBeg == size_t(-1) )
      throw std::logic_error( "attempt of adding line to closed markup" );

    if ( len == size_t(-1) )
      for ( ++len; str[len] != 0; ++len ) (void)NULL;

    if ( (last = LastMarkup( this )) != nullptr )
      last->Close();

    mtc::w_strncpy( pstr = rebind<widechar>( docptr->lines.get_allocator() ).allocate( len + 1 ),
      str, len )[len] = 0;

    docptr->lines.push_back( { unsigned(-1), len, pstr } );
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

# endif   // !__palmira_DOM_text_hpp__; ++spansPtr->uUpper
