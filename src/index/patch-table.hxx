# include "../../api/span.hxx"
# include "../tools/primes.hxx"
# include <mtc/ptrpatch.h>
# include <string_view>
# include <cstring>
# include <vector>
# include <atomic>

namespace palmira {
namespace index   {

  template <class Allocator = std::allocator<char>>
  class PatchTable
  {
    class PatchVal;
    class PatchRec;

    using HashItem = std::atomic<PatchRec*>;

    template <class Target>
    using MakeAllocator = typename std::allocator_traits<Allocator>::rebind_alloc<Target>;

  public:
    class PatchAPI;

  public:
    PatchTable( size_t maxPatches, Allocator alloc = Allocator() );
   ~PatchTable();

  public:
    auto  Update( const Span& id, const Span& md ) -> PatchAPI
      {  return Modify( id, PatchVal::Create( md, hashTable.get_allocator() ) );  }
    void  Delete( const Span& id )
      {  return (void)Modify( id, { PatchVal::deleted(), hashTable.get_allocator() } );  }
    auto  Search( const Span& ) -> PatchAPI;
    auto  Update( uint32_t ix, const Span& md ) -> PatchAPI {  return Update( MakeId( ix ), md );  }
    void  Delete( uint32_t ix )                             {  return Delete( MakeId( ix ) );  }
    auto  Search( uint32_t ix ) -> PatchAPI                 {  return Search( MakeId( ix ) );  }

    auto  Modify( const Span&, const PatchAPI& ) -> PatchAPI;
  protected:
    static  auto  MakeId( uint32_t ix ) -> Span;
    static  auto  HashId( const Span& ) -> size_t;

  protected:
    std::vector<HashItem, Allocator>  hashTable;

  };

  template <class Allocator>
  class PatchTable<Allocator>::PatchVal   // record keeping the serialized value
  {
    friend class PatchTable;

    std::atomic_long  rcount = 0;
    size_t            length;

    constexpr static PatchVal* deleted() noexcept {  return (PatchVal*)(uintptr_t(nullptr) - 1); }

  public:
    static
    auto  Create( const Span&, Allocator ) -> PatchAPI;
    void  Delete( Allocator );

  public:
    char*   data() const {  return (char*)(this + 1);  }
    size_t  size() const {  return length;  }
  };

  template <class Allocator>
  class PatchTable<Allocator>::PatchRec
  {
    friend class PatchTable;

  public:
    PatchRec( PatchRec* ptr, const Span& key, PatchVal* val ):
      collision( ptr ),
      entityKey( key ),
      patchData( val )
    {
      if ( val != nullptr && val != PatchVal::deleted() )
        ++patchData.load()->rcount;
    }

  public:
    // collision resolving comparison
    bool  operator == ( const Span& to ) const;
    bool  operator != ( const Span& to ) const  {  return !(*this == to);  }

    // helper func
    auto  Modify( PatchAPI ) -> PatchAPI;

  protected:
    std::atomic<PatchRec*>  collision = nullptr;
    Span                    entityKey = { nullptr, 0 };
    std::atomic<PatchVal*>  patchData = nullptr;

  };

  template <class Allocator>
  class PatchTable<Allocator>::PatchAPI
  {
    friend class PatchTable;

    void  Detach()
    {
      if ( ptr != nullptr && ptr != PatchVal::deleted() )
        ptr->Delete( mem );
    }

    PatchAPI( PatchVal* p, Allocator a ): mem( a ), ptr( p )
    {
      if ( ptr != nullptr && ptr != PatchVal::deleted() )
        ++ptr->rcount;
    }

  public:
    PatchAPI() = default;
    PatchAPI( const PatchAPI& p ): mem( p.mem ), ptr( p.ptr )
    {
      if ( ptr != nullptr && ptr != PatchVal::deleted() )
        ++ptr->rcount;
    }
   ~PatchAPI()
    {
      Detach();
    }
    PatchAPI& operator = ( const PatchAPI& p )
    {
      Detach();
      if ( (ptr = p.ptr) != nullptr && ptr != PatchVal::deleted() )
        ++ptr->rcount;
      return *this;
    }

    auto  data() const -> const char* {  return ptr != nullptr ? ptr->data() : nullptr; }
    auto  size() const -> size_t {  return ptr != nullptr ? ptr->size() : 0;  }

    bool  operator == ( nullptr_t ) const {  return ptr == nullptr;  }
    bool  operator != ( nullptr_t ) const {  return !(*this == nullptr);  }

    bool  IsDeleted() const {  return ptr == PatchVal::deleted(); }
    bool  IsUpdated() const {  return ptr != PatchVal::deleted(); }

    auto  get_allocator() const -> Allocator  {  return mem;  }
    auto  get() const -> PatchVal* {  return (PatchVal*)ptr; }
    auto  release() -> PatchVal* {  auto p = ptr;  ptr = nullptr;  return p; }

  protected:
    Allocator  mem;
    PatchVal*  ptr = nullptr;

  };

  // PatchVal implementation

  template <class Allocator>
  auto  PatchTable<Allocator>::PatchVal::Create( const Span& data, Allocator mman ) -> PatchAPI
  {
    auto  nalloc = (sizeof(PatchVal) * 2 + data.size() - 1) / sizeof(PatchVal);
    auto  palloc = new( MakeAllocator<PatchVal>( mman ).allocate( nalloc ) )
      PatchVal();

    memcpy( palloc->data(), data.data(), palloc->length = data.size() );

    return { palloc, mman };
  }

  template <class Allocator>
  void  PatchTable<Allocator>::PatchVal::Delete( Allocator alloc )
  {
    if ( --rcount == 0 )
    {
      this->~PatchVal();
      MakeAllocator<PatchVal>( alloc ).deallocate( this, 0 );
    }
  }

  // PatchTable::PatchRec implementation

  template <class Allocator>
  bool  PatchTable<Allocator>::PatchRec::operator == ( const Span& to ) const
  {
    if ( entityKey.data() == to.data() )
      return true;

    if ( (uintptr_t(to.data()) & uintptr_t(entityKey.data())) >> 32 == uint32_t(-1) )
      return uint32_t(uintptr_t(entityKey.data())) == uint32_t(uintptr_t(to.data()));

    if ( (uintptr_t(entityKey.data()) >> 32) == uint32_t(-1) || (uintptr_t(to.data()) >> 32) == uint32_t(-1) )
      return false;

    return std::string_view( entityKey.data(), entityKey.size() )
        == std::string_view( to.data(), to.size() );
  }

 /*
  * PatchTable<Allocator>::PatchRec::Delete()
  *
  * Set the PatchVal pointer of PatchRec to ptr(-1) to mark as deleted object.
  *
  * If there is an existing patch data, deletes it if no objects refer.
  */
  /*
   * PatchTable<Allocator>::PatchRec::Update( PatchAPI )
   *
   * Set the PatchVal data to value passed.
   * If there is an existing patch data, updates it.
   * Does not override Deleted().
   */
  template <class Allocator>
  auto  PatchTable<Allocator>::PatchRec::Modify( PatchAPI data ) -> PatchAPI
  {
    auto  pvalue = patchData.load();

    while ( pvalue != PatchVal::deleted() && !patchData.compare_exchange_weak( pvalue, data.get() ) )
      (void)NULL;

    if ( pvalue != PatchVal::deleted() )
    {
      auto  result = data;

      if ( pvalue != nullptr )
        pvalue->Delete( data.get_allocator() );

      return data.release(), result;
    }
    return { pvalue, data.get_allocator() };
  }

  // PatchTable implementation
  template <class Allocator>
  PatchTable<Allocator>::PatchTable( size_t maxPatches, Allocator alloc ):
    hashTable( UpperPrime( maxPatches ), alloc )
  {
  }

  template <class Allocator>
  PatchTable<Allocator>::~PatchTable()
  {
    auto  entryAlloc = MakeAllocator<PatchRec>( hashTable.get_allocator() );
    auto  patchAlloc = MakeAllocator<PatchVal>( hashTable.get_allocator() );

    for ( auto& rentry: hashTable )
      for ( auto pentry = mtc::ptr::clean( rentry.load() ); pentry != nullptr; )
      {
        auto  pfetch = pentry->collision.load();
        auto  pvalue = pentry->patchData.load();

        if ( pvalue != nullptr && pvalue != PatchVal::deleted() )
          pvalue->Delete( patchAlloc );

        pentry->~PatchRec();
        entryAlloc.deallocate( pentry, 0 );

        pentry = pfetch;
      }
  }

  /*
  * Creates a metadata update record with no override for 'deleted'
  */
  template <class Allocator>
  auto  PatchTable<Allocator>::Modify( const Span& key, const PatchAPI& pvalue ) -> PatchAPI
  {
    auto& rentry = hashTable[HashId( key ) % hashTable.size()];
    auto  pentry = mtc::ptr::clean( rentry.load() );

    // check if delete existing element
    for ( ; pentry != nullptr; pentry = mtc::ptr::clean( pentry->collision.load() ) )
      if ( *pentry == key )
        return pentry->Modify( pvalue );

    // try lock the hash entry
    for ( pentry = mtc::ptr::clean( rentry.load() ); !rentry.compare_exchange_weak( pentry, mtc::ptr::dirty( pentry ) ); )
      pentry = mtc::ptr::clean( pentry );

    // lookup the deletion record again
    for ( ; pentry != nullptr; pentry = mtc::ptr::clean( pentry->collision.load() ) )
      if ( *pentry == key )
      {
        rentry.store( mtc::ptr::clean( rentry.load() ) );
        return pentry->Modify( pvalue );
      }

    // allocate entry record and store to table
    try
    {
      pentry = new ( MakeAllocator<PatchRec>( hashTable.get_allocator() ).allocate( 1 ) )
        PatchRec( mtc::ptr::clean( rentry.load() ), key, pvalue.get() );
      rentry.store( pentry );
      return pvalue;
    }
    catch ( ... )
    {
      rentry.store( pentry );
      throw;
    }
  }

  template <class Allocator>
  auto  PatchTable<Allocator>::Search( const Span& key ) -> PatchAPI
  {
    auto& rentry = hashTable[HashId( key ) % hashTable.size()];
    auto  pentry = mtc::ptr::clean( rentry.load() );

  // check if element exists
    for ( ; pentry != nullptr; pentry = mtc::ptr::clean( pentry->collision.load() ) )
      if ( *pentry == key )
        return { pentry->patchData.load(), hashTable.get_allocator() };

    return { nullptr, hashTable.get_allocator() };
  }

  template <class Allocator>
  auto  PatchTable<Allocator>::HashId( const Span& key ) -> size_t
  {
    static_assert( sizeof(uintptr_t) > sizeof(uint32_t),
      "this code is designed for at least 64-bit pointers" );

    if ( (uintptr_t(key.data()) >> 32) == uint32_t(-1) )
      return uintptr_t(key.data()) & 0xffffffffUL;

    return std::hash<std::string_view>()( { key.data(), key.size() } );
  }

  template <class Allocator>
  auto  PatchTable<Allocator>::MakeId( uint32_t ix ) -> Span
  {
    static_assert( sizeof(uintptr_t) > sizeof(uint32_t),
      "this code is designed for at least 64-bit pointers" );
    return Span( (const char*)(0xffffffff00000000LU | ix), 0 );
  }

}}