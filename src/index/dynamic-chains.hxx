# if !defined( __palmira_src_index_dynamic_chains_hxx__ )
# define __palmira_src_index_dynamic_chains_hxx__
# include "../../api/index.hxx"
# include <atomic>
#include <bits/fs_fwd.h>

# define LineId( arg )  "" #arg

namespace palmira {
namespace index   {
namespace dynamic {

  template <class Allocator = std::allocator<char>>
  class KeyBlockChains
  {
    struct ChainLink;
    struct ChainHook;

    using AtomicLink = std::atomic<ChainLink*>;
    using AtomicHook = std::atomic<ChainHook*>;

    using LinkAllocator = typename std::allocator_traits<Allocator>::rebind_alloc<ChainLink>;
    using HookAllocator = typename std::allocator_traits<Allocator>::rebind_alloc<ChainHook>;

    enum: size_t
    {
      hash_table_size = 10501
    };

  /*
   * ChainLink represents one entity identified by virtual index iEnt data block
   * linked in a chain of blocks in increment order of entity indices
   */
    struct ChainLink
    {
      AtomicLink  p_next = nullptr;
      uint32_t    entity;
      uint32_t    lblock;

    public:
      ChainLink( uint32_t ent, std::string_view blk ): entity( ent )
        {  memcpy( data(), blk.data(), lblock = blk.size() );  }

    public:
      auto  data() const -> const char*  {  return (const char*)(this + 1);  }
      auto  data() ->             char*  {  return (      char*)(this + 1);  }

    };

  /*
   * ChainHook holds key body and reference to the first element in the chain
   * of blocks indexed by incremental virtual entity indices
   */
    struct ChainHook
    {
      using LastAnchor = std::atomic<AtomicLink*>;

      enum: size_t
      {
        cache_size = 32,
        cache_step = 64
      };

      LinkAllocator       malloc;
      AtomicHook          pchain;               // collisions

      AtomicLink          pfirst = nullptr;     // first in chain
      AtomicLink          points[cache_size];   // points cache
      LastAnchor          ppoint = points - 1;  // invalid value
      std::atomic_size_t  ncount = 0;

      size_t              cchkey;               // key length

    protected:
      auto  data() const -> const char* {  return (const char*)(this + 1);  }
      auto  data() -> char* {  return (char*)(this + 1);  }

    public:
      ChainHook( std::string_view, ChainHook*, Allocator );
     ~ChainHook();

    public:
      bool  operator == ( const std::string_view& s ) const
        {  return cchkey == s.size() && memcmp( data(), s.data(), s.size() ) == 0;  }

    public:
      void  Insert( uint32_t, std::string_view );
      void  Markup();
     /*
      * bool  Verify() const;
      *
      * for tesing:
      *   verifies the chain of entities to increment values
      *   element to element
      */
      bool  Verify() const;

    };

  public:
    KeyBlockChains( Allocator alloc = Allocator() ):
      hashTable( hash_table_size, alloc ),
      hookAlloc( alloc )  {}
   ~KeyBlockChains();

  public:
    void  Insert( std::string_view, uint32_t, std::string_view );
    auto  Lookup( std::string_view s ) const -> const ChainHook*;
   /*
    * bool  Verify() const;
    *
    * for tesing: lists all the keys and checks integrity
    */
    bool  Verify() const;

  protected:
    std::vector<AtomicHook, Allocator>  hashTable;
    HookAllocator                       hookAlloc;

  };

// KeyBlockChains template implementation

  template <class Allocator>
  KeyBlockChains<Allocator>::~KeyBlockChains()
  {
    for ( auto& next: hashTable )
      for ( auto tostep = next.load(), tofree = tostep; tofree != nullptr; tofree = tostep )
      {
        tostep = tofree->pchain.load();
          tofree->~ChainHook();
        hookAlloc.deallocate( tofree, 0 );
      }
  }

  template <class Allocator>
  void KeyBlockChains<Allocator>::Insert( std::string_view key, uint32_t entity, std::string_view block )
  {
    auto  hindex = std::hash<std::string_view>{}( key ) % hashTable.size();
    auto  hentry = &hashTable[hindex];
    auto  hvalue = mtc::ptr::clean( hentry->load() );

  // first try find existing block in the hash chain
    while ( hvalue != nullptr )
      if ( *hvalue == key ) return hvalue->Insert( entity, block );
        else hvalue = hvalue->pchain.load();

  // now try lock the hash table entry to create record
    for ( hvalue = mtc::ptr::clean( hentry->load() ); !hentry->compare_exchange_weak( hvalue, mtc::ptr::dirty( hvalue ) ); )
      hvalue = mtc::ptr::clean( hvalue );

  // check if locked hvalue list still contains no needed key; unlock and finish
  // if key is now found
    while ( hvalue != nullptr )
      if ( *hvalue == key )
      {
        hentry->store( mtc::ptr::clean( hentry->load() ) );
        return hvalue->Insert( entity, block );
      } else hvalue = hvalue->pchain.load();

  // list contains no needed entry; allocate new ChainHook for new key;
  // for possible exceptions, unlock the entry and continue tracing
    try
    {
      new( hvalue = hookAlloc.allocate( (sizeof(ChainHook) * 2 + key.size() - 1) / sizeof(ChainHook) ) )
        ChainHook( key, mtc::ptr::clean( hentry->load() ), hookAlloc );
      hentry->store( hvalue );
    }
    catch ( ... )
    {
      hentry->store( mtc::ptr::clean( hentry->load() ) );
      throw;
    }

    return hvalue->Insert( entity, block );
  }

  template <class Allocator>
  auto  KeyBlockChains<Allocator>::Lookup( std::string_view key ) const -> const ChainHook*
  {
    auto  hindex = std::hash<std::string_view>{}( key ) % hashTable.size();
    auto  hvalue = mtc::ptr::clean( hashTable[hindex].load() );

  // first try find existing block in the hash chain
    for ( ; hvalue != nullptr; hvalue->pchain.load() )
      if ( *hvalue == key )
        return hvalue;

    return nullptr;
  }

  template <class Allocator>
  bool  KeyBlockChains<Allocator>::Verify() const
  {
    for ( auto& next: hashTable )
      for ( auto verify = next.load(); verify != nullptr; verify = verify->pchain.load() )
        if ( !verify->Verify() )
          return false;

    return true;
  }

  // KeyBlockChains::ChainHook template implementation

  template <class Allocator>
  KeyBlockChains<Allocator>::ChainHook::ChainHook( std::string_view key, ChainHook* p, Allocator m ):
    malloc( m ),
    pchain( p )
  {
    memcpy( data(), key.data(), cchkey = key.size() );
  }

  template <class Allocator>
  KeyBlockChains<Allocator>::ChainHook::~ChainHook()
  {
    for ( auto pnext = pfirst.load(), pfree = pnext; pfree != nullptr; pfree = pnext )
    {
      pnext = pfree->p_next.load();
        pfree->~ChainLink();
      malloc.deallocate( pfree, 0 );
    }
  }

  template <class Allocator>
  void  KeyBlockChains<Allocator>::ChainHook::Insert( uint32_t entity, std::string_view block )
  {
    auto        newptr = new( malloc.allocate( (sizeof(ChainLink) * 2 + block.size() - 1) / sizeof(ChainLink) ) )
      ChainLink( entity, block );
    ChainLink*  pentry;
    AtomicLink* anchor;

  // check if no elements available and try write first element;
  // if succeeded, increment element count and return
    if ( pfirst.compare_exchange_weak( pentry = nullptr, newptr ) )
      return (void)++ncount;

  // now pentry has the value != nullptr that was stored in pfirst
  // on the moment of a call
  //  if ( pentry == nullptr )
  //    throw std::logic_error( "algorithm error @" __FILE__ ":" LineId( __LINE__ ) );

  // check if index is not being built this moment and get the latest
  // insert position
    if ( (mtc::ptr::toint( anchor = ppoint.load() ) & 0x01) == 0 )
    {
      while ( anchor >= points && (pentry = (anchor--)->load())->entity >= entity )
        (void)NULL;
      if ( pentry->entity >= entity )
        pentry = pfirst.load();
    }

  // check if the element inserted < than the first element stored;
  // try prepend current element to the existing list
    while ( entity < pentry->entity )
    {
      if ( newptr->p_next = pentry, pfirst.compare_exchange_weak( pentry, newptr ) )
        return (++ncount % cache_step) == 0 ? Markup() : (void)NULL;
    }

  // now the element inserted is > pentry;
  //  if ( pentry == nullptr || entity <= pentry->entity )
  //    throw std::logic_error( "algorithm error @" __FILE__ ":" LineId( __LINE__ ) );

  // follow the list searching for element to be appended to current element
    for ( ; ; )
    {
      auto  follow = pentry->p_next.load();

      if ( follow == nullptr || follow->entity >= entity )
      {
        newptr->p_next = follow;

        if ( pentry->p_next.compare_exchange_weak( follow, newptr ) )
          return (++ncount % cache_step) == 0 ? Markup() : (void)NULL;
      }
      if ( follow->entity <= entity )
        pentry = follow;
    }
  }

  template <class Allocator>
  void  KeyBlockChains<Allocator>::ChainHook::Markup()
  {
    auto  n_gran = ncount.load() / cache_size;
    auto  pentry = pfirst.load();
    auto  pcache = mtc::ptr::clean( ppoint.load() );

    // ensure only one cache builder
    if ( ppoint.compare_exchange_weak( pcache, mtc::ptr::dirty( pcache ) ) ) pcache = points;
      else return;

    for ( size_t nindex = 0; pentry != nullptr; pentry = pentry->p_next.load() )
      if ( nindex++ == n_gran )
      {
        *pcache++ = pentry;
        nindex = 0;
      }
    ppoint = pcache - 1;
  }

  template <class Allocator>
  bool  KeyBlockChains<Allocator>::ChainHook::Verify() const
  {
    auto  entity = uint32_t(0);

    for ( auto pentry = pfirst.load(); pentry != nullptr; pentry = pentry->p_next.load() )
      if ( pentry->entity <= entity )
        return false;
      else entity = pentry->entity;

    return true;
  }

}}}

# endif   // __palmira_src_index_dynamic_chains_hxx__
