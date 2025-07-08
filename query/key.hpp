# if !defined( __palmira_query_key_hpp__ )
# define __palmira_query_key_hpp__
# include <mtc/serialize.h>
# include <mtc/wcsstr.h>
# include <stdexcept>
# include <cstdint>

namespace palmira {
namespace query {

  template <class Allocator>
  class basic_key
  {
    enum: unsigned
    {
      is_string = 1,
      has_class = 2
    };
    struct memkey
    {
      int       rcount;
      Allocator malloc;
    };

    char  keybuf[sizeof(char*) * 3];
    char* keyptr;

    template <class A>
    using wide_string = std::basic_string<widechar, std::char_traits<widechar>, A>;

    template <class I>
    static  int   valuelen( I );
    static  char* writeint( char*, uint16_t );
    static  char* writeint( char*, uint32_t );
    static  char* writeint( char*, uint64_t );
    static  auto  loadfrom( const char*, uint32_t& ) -> const char*;

  public:
    basic_key();
    basic_key( basic_key&& );
    basic_key( const basic_key& );
    basic_key( unsigned, uint32_t );
    basic_key( unsigned, uint64_t );
    basic_key( unsigned, uint32_t, const widechar*, size_t, Allocator = Allocator() );
    basic_key( unsigned, const widechar*, size_t, Allocator = Allocator() );
  template <class OtherAllocator>
    basic_key( unsigned, uint32_t, const wide_string<OtherAllocator>&, Allocator = Allocator() );
  template <class OtherAllocator>
    basic_key( unsigned, const wide_string<OtherAllocator>&, Allocator = Allocator() );
   ~basic_key();

    basic_key& operator=( basic_key&& );
    basic_key& operator=( const basic_key& );

  public:
    void  clear();

    auto  data() const -> const char*
    {
      if ( keyptr != nullptr )
        return ::SkipToEnd( (const char*)keyptr, (unsigned*)nullptr );
      return nullptr;
    }
    auto  size() const -> size_t
    {
      unsigned  cchkey;

      if ( keyptr != nullptr )
        return ::FetchFrom( (const char*)keyptr, cchkey ), cchkey;
      return 0;
    }

    bool  has_int() const;
    bool  has_str() const;
    bool  has_cls() const;

    auto  get_idl() const -> unsigned;
    auto  get_int() const -> uint64_t;
    auto  get_cls() const -> uint32_t;
    auto  get_str( widechar*, size_t ) const -> const widechar*;
    auto  get_len() const -> size_t;

  };

  using key = basic_key<std::allocator<char>>;

  // basic_key template implementation

  template <class Allocator>
  template <class U>
  int   basic_key<Allocator>::valuelen( U u )
  {
    return u <= 0x0000007f ? 1 :
           u <= 0x000007ff ? 2 :
           u <= 0x0000ffff ? 3 :
           u <= 0x001fffff ? 4 :
           u <= 0x03ffffff ? 5 :
           u <= 0x7fffffff ? 6 :
           u <= 0x0fffffffff ? 7 :
           u <= 0x03ffffffffff ? 8 : (throw std::invalid_argument( "value too big to be presented as utf-8" ), 0);
  }

  template <class Allocator>
  char* basic_key<Allocator>::writeint( char* p, uint16_t n )
  {
    if ( (n & ~0x007f) == 0 )
    {
      *p++ = (char)(n & 0x7f);
    }
      else
    if ( (n & ~0x07ff) == 0 )
    {
      *p++ = 0xc0 | ((char)(n >> 0x06));
      *p++ = 0x80 | ((char)(n & 0x3f));
    }
      else
    {
      *p++ = 0xe0 | ((char)(n >> 0x0c));
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
    return p;
  }

  template <class Allocator>
  char* basic_key<Allocator>::writeint( char* p, uint32_t n )
  {
    if ( (n & ~0x0000ffff) == 0 )
      return writeint( p, (uint16_t)n );
    if ( (n & ~0x001fffff) == 0 )
    {
      *p++ = 0xf0 | ((char)(n >> 0x12));
      *p++ = 0x80 | ((char)(n >> 0x0c) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
      else
    if ( (n & ~0x03ffffff) == 0 )
    {
      *p++ = 0xf8 | ((char)(n >> 0x18));
      *p++ = 0x80 | ((char)(n >> 0x12) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x0c) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
      else
    if ( (n & ~0x7fffffff) == 0 )
    {
      *p++ = 0xfc | ((char)(n >> 0x1e));
      *p++ = 0x80 | ((char)(n >> 0x18) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x12) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x0c) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
      else
    {
      *p++ = (char)0xfe;
      *p++ = 0x80 | ((char)(n >> 0x1e) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x18) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x12) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x0c) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
    return p;
  }

  template <class Allocator>
  char* basic_key<Allocator>::writeint( char* p, uint64_t n )
  {
    if ( (n & ~uint64_t(0x0ffffffff)) == 0 )
      return writeint( p, (uint32_t)n );
    if ( (n & ~uint64_t(0x0fffffffff)) == 0 )
    {
      *p++ = (char)0xfe;
      *p++ = 0x80 | ((char)(n >> 0x1e) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x18) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x12) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x0c) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
      else
    if ( (n & ~uint64_t(0x03ffffffffff)) == 0 )
    {
      *p++ = (char)0xff;
      *p++ = 0x80 | ((char)(n >> 0x24) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x1e) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x18) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x12) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x0c) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x06) & 0x3f);
      *p++ = 0x80 | ((char)(n >> 0x00) & 0x3f);
    }
      else
    throw std::invalid_argument( "value too big to be presented as utf-8" );
    return p;
  }

  template <class Allocator>
  auto  basic_key<Allocator>::loadfrom( const char* p, uint32_t& v ) -> const char*
  {
    uint8_t   ctlchr = (uint8_t)*p++;

    if ( ctlchr == 0xfe )
    {
      v = ((uint32_t)(uint8_t)(p[0] & 0x3f)) << 0x1e
        | ((uint32_t)(uint8_t)(p[1] & 0x3f)) << 0x18
        | ((uint32_t)(uint8_t)(p[2] & 0x3f)) << 0x12
        | ((uint32_t)(uint8_t)(p[3] & 0x3f)) << 0x0c
        | ((uint32_t)(uint8_t)(p[4] & 0x3f)) << 0x06
        | ((uint32_t)(uint8_t)(p[5] & 0x3f));
      return p + 6;
    }

    if ( (ctlchr & 0xfe) == 0xfc )
    {
      v = (ctlchr & 0x01) << 0x1e
        | ((uint32_t)(uint8_t)(p[0] & 0x3f)) << 0x18
        | ((uint32_t)(uint8_t)(p[1] & 0x3f)) << 0x12
        | ((uint32_t)(uint8_t)(p[2] & 0x3f)) << 0x0c
        | ((uint32_t)(uint8_t)(p[3] & 0x3f)) << 0x06
        | ((uint32_t)(uint8_t)(p[4] & 0x3f));
      return p + 5;
    }

    if ( (ctlchr & 0xfc) == 0xf8 )
    {
      v = (ctlchr & 0x03) << 0x18
        | ((uint32_t)(uint8_t)(p[0] & 0x3f)) << 0x12
        | ((uint32_t)(uint8_t)(p[1] & 0x3f)) << 0x0c
        | ((uint32_t)(uint8_t)(p[2] & 0x3f)) << 0x06
        | ((uint32_t)(uint8_t)(p[3] & 0x3f));
      return p + 4;
    }

    if ( (ctlchr & 0xf8) == 0xf0 )
    {
      v = (ctlchr & 0x07) << 0x12
        | ((uint32_t)(uint8_t)(p[0] & 0x3f)) << 0x0c
        | ((uint32_t)(uint8_t)(p[1] & 0x3f)) << 0x06
        | ((uint32_t)(uint8_t)(p[2] & 0x3f));
      return p + 3;
    }

    if ( (ctlchr & 0xf0) == 0xe0 )
    {
      v = (ctlchr & 0x0f) << 0x0c
        | ((uint32_t)(uint8_t)(p[0] & 0x3f)) << 0x06
        | ((uint32_t)(uint8_t)(p[1] & 0x3f));
      return p + 2;
    }

    if ( (ctlchr & 0xe0) == 0xc0 )
    {
      v = (ctlchr & 0x1f) << 0x06
        | ((uint32_t)(uint8_t)(*p & 0x3f));
      return p + 1;
    }

    if ( (ctlchr & 0x80) == 0 )
    {
      return (v = ctlchr), p;
    }

    return nullptr;
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key(): keyptr( nullptr )
  {
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key( basic_key&& key )
  {
    if ( (keyptr = key.keyptr) != nullptr )
    {
      if ( keyptr == key.keybuf )
        keyptr = (char*)memcpy( keybuf, key.keybuf, sizeof(keybuf) );
      key.keybuf = nullptr;
    }
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key( const basic_key& key )
  {
    if ( (keyptr = key.keyptr) != nullptr )
    {
      if ( keyptr != key.keybuf ) ++(-1 + (memkey*)keyptr)->rcount;
        else keyptr = (char*)memcpy( keybuf, key.keybuf, sizeof(keybuf) );
    }
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key( unsigned idl, uint32_t lex )
  {
    auto  keylen = valuelen( idl << 2 ) + valuelen( lex );
    auto  keyout = keyptr = keybuf;

    writeint( writeint( ::Serialize( keyout, keylen ), idl << 2 ), lex );
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key( unsigned idl, uint64_t lex )
  {
    auto  keylen = valuelen( idl << 2 ) + valuelen( lex );
    auto  keyout = keyptr = keybuf;

    writeint( writeint( ::Serialize( keyout, keylen ), idl << 2 ), lex );
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key( unsigned idl, uint32_t cls, const widechar* str, size_t len, Allocator mem )
  {
    auto    keylen = valuelen( (idl << 2) | is_string | has_class ) + valuelen( cls ) + valuelen( 0 );
    size_t  nalloc;
    char*   outptr = keyptr = keybuf;

    if ( str == nullptr || len == 0 )
      throw std::invalid_argument( "key string is empty" );

    if ( len == size_t(-1) )
      for ( len = 0; str[len] != 0; ++len ) (void)NULL;

    for ( auto s = str; s != str + len; ++s )
      keylen += valuelen( *s );

    if ( (nalloc = ::GetBufLen( keylen ) + keylen) > sizeof(keybuf) )
    {
      auto  malloc = typename std::allocator_traits<Allocator>::template rebind_alloc<memkey>( mem );
      auto  nitems = (nalloc + sizeof(memkey) * 2 - 1) / sizeof(memkey);
      auto  memptr = malloc.allocate( nitems );

      outptr = keyptr = (char*)(1 + memptr);
        memptr->rcount = 1;
    }

    for ( outptr = writeint( ::Serialize( outptr, keylen ), (idl << 2)  | is_string | has_class ); len-- != 0; ++str )
      outptr = writeint( outptr, *str );

    writeint( writeint( outptr, uint16_t(0) ), cls );
  }

  template <class Allocator>
  basic_key<Allocator>::basic_key( unsigned idl, const widechar* str, size_t len, Allocator mem )
  {
    auto    keylen = valuelen( (idl << 2) | is_string ) + valuelen( 0 );
    size_t  nalloc;
    char*   outptr = keyptr = keybuf;

    if ( str == nullptr || len == 0 )
      throw std::invalid_argument( "key string is empty" );

    if ( len == size_t(-1) )
      for ( len = 0; str[len] != 0; ++len ) (void)NULL;

    for ( auto s = str; s != str + len; ++s )
      keylen += valuelen( *s );

    if ( (nalloc = ::GetBufLen( keylen ) + keylen) > sizeof(keybuf) )
    {
      auto  malloc = typename std::allocator_traits<Allocator>::template rebind_alloc<memkey>( mem );
      auto  nitems = (nalloc + sizeof(memkey) * 2 - 1) / sizeof(memkey);
      auto  memptr = malloc.allocate( nitems );

      outptr = keyptr = (char*)(1 + memptr);
      memptr->rcount = 1;
    }

    for ( outptr = writeint( ::Serialize( outptr, keylen ), (idl << 2) | is_string ); len-- != 0; ++str )
      outptr = writeint( outptr, *str );

    writeint( outptr, uint16_t(0) );
  }

  template <class Allocator>
  template <class OtherAllocator>
  basic_key<Allocator>::basic_key( unsigned idl, uint32_t cls, const wide_string<OtherAllocator>& str, Allocator mem ):
    basic_key( idl, cls, str.data(), str.size(), mem ) {}

  template <class Allocator>
  template <class OtherAllocator>
  basic_key<Allocator>::basic_key( unsigned idl, const wide_string<OtherAllocator>& str, Allocator mem ):
    basic_key( idl, str.data(), str.size(), mem ) {}

  template <class Allocator>
  basic_key<Allocator>::~basic_key()
  {
    clear();
  }

  template <class Allocator>
  auto  basic_key<Allocator>::operator=( basic_key&& key ) -> basic_key<Allocator>&
  {
    clear();

    if ( (keyptr = key.keyptr) != nullptr )
    {
      if ( keyptr == key.keybuf )
        keyptr = (char*)memcpy( keybuf, key.keybuf, sizeof(keybuf) );
      key.keyptr = nullptr;
    }
    return *this;
  }

  template <class Allocator>
  auto  basic_key<Allocator>::operator=( const basic_key& key ) -> basic_key<Allocator>&
  {
    clear();

    if ( (keyptr = key.keyptr) != nullptr )
    {
      if ( keyptr == key.keybuf ) ++(-1 + (memkey*)keyptr)->rcount;
        else keyptr = (char*)memcpy( keybuf, key.keybuf, sizeof(keybuf) );
    }
    return *this;
  }

  template <class Allocator>
  void  basic_key<Allocator>::clear()
  {
    if ( keyptr != nullptr && keyptr != keybuf )
    {
      auto  ptrmem = ((memkey*)keyptr) - 1;

      if ( --ptrmem->rcount == 0 )
      {
        auto  malloc = typename std::allocator_traits<Allocator>::template rebind_alloc<memkey>(
          ptrmem->malloc );
        malloc.deallocate( ptrmem, 0 );
      }
    }
    keyptr = nullptr;
  }

  template <class Allocator>
  bool  basic_key<Allocator>::has_int() const
  {
    uint32_t  idl;

    if ( keyptr != nullptr )
    {
      return loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl ) != nullptr ?
        (idl & (is_string | has_class)) == 0 : false;
    }
    return false;
  }

  template <class Allocator>
  bool  basic_key<Allocator>::has_str() const
  {
    uint32_t  idl;

    if ( keyptr != nullptr )
    {
      return loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl ) != nullptr ?
        (idl & is_string) != 0 : false;
    }
    return false;
  }

  template <class Allocator>
  bool  basic_key<Allocator>::has_cls() const
  {
    uint32_t  idl;

    if ( keyptr != nullptr )
    {
      return loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl ) != nullptr ?
        (idl & has_class) != 0 : false;
    }
    return false;
  }

  template <class Allocator>
  auto  basic_key<Allocator>::get_idl() const -> unsigned
  {
    uint32_t  idl;

    if ( keyptr != nullptr )
      if ( loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl ) != nullptr )
        return idl >> 2;

    return unsigned(-1);
  }

  template <class Allocator>
  auto  basic_key<Allocator>::get_int() const -> uint64_t
  {
    uint32_t    idl;
    uint32_t    lex;
    const char* src;

    if ( keyptr != nullptr )
      if ( (src = loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl )) != nullptr )
        if ( (idl & (is_string | has_class)) == 0 && loadfrom( src, lex ) != nullptr )
          return lex;

    return 0;
  }

  template <class Allocator>
  auto  basic_key<Allocator>::get_cls() const -> uint32_t
  {
    uint32_t    idl;
    uint32_t    cls;
    const char* src;

    if ( keyptr == nullptr )
      return 0;

    if ( (src = loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl )) == nullptr )
      return 0;

    if ( (idl & has_class) == 0 )
      return 0;

    if ( (idl & is_string) != 0 )
      while ( (src = loadfrom( src, cls )) != nullptr && cls != 0 )
        (void)NULL;

    return src != nullptr && (src = loadfrom( src, cls )) != nullptr ? cls : 0;
  }

  template <class Allocator>
  auto  basic_key<Allocator>::get_str( widechar* out, size_t max ) const -> const widechar*
  {
    uint32_t    idl;
    size_t      len;
    uint32_t    chr;
    const char* src;

    // check the arguments
    if ( out == nullptr || max == 0 )
      throw std::invalid_argument( "string buffer is empty" );

    // check if has anything
    if ( keyptr == nullptr )
      return nullptr;

    // check if has string
    if ( (src = loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl )) != nullptr )
      if ( (idl & is_string) == 0 )
        return nullptr;

    // get string
    for ( len = 0; len != max && (src = loadfrom( src, chr )) != nullptr && chr != 0; )
      out[len++] = chr;

    return src != nullptr && len < max ? out[len] = 0, out : nullptr;
  }

  template <class Allocator>
  auto  basic_key<Allocator>::get_len() const -> size_t
  {
    uint32_t    idl;
    size_t      len;
    unsigned    chr;
    const char* src;

    if ( keyptr == nullptr )
      return 0;

  // check if has string
    if ( (src = loadfrom( ::SkipToEnd( (const char*)keyptr, (int*)nullptr ), idl )) != nullptr )
      if ( (idl & is_string) == 0 )
        return 0;

  // get string length
    for ( len = 0; (src = loadfrom( src, chr )) != nullptr && chr != 0; ++len )
      (void)NULL;

    return src != nullptr ? len : 0;
  }

}}

# endif // __palmira_query_key_hpp__
