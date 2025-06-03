# if !defined( __palmira_src_index_dynamic_entities_hxx__ )
# define __palmira_src_index_dynamic_entities_hxx__
# include "../../api/contents-index.hxx"
# include "../tools/primes.hxx"
# include <mtc/ptrpatch.h>
# include <mtc/wcsstr.h>
# include <type_traits>
# include <algorithm>
# include <stdexcept>
# include <vector>
# include <string>
# include <atomic>

namespace palmira {
namespace index   {
namespace dynamic {

  class index_overflow: public std::runtime_error {  using runtime_error::runtime_error;  };
  class count_overflow: public index_overflow {  using index_overflow::index_overflow;  };

  template <class Allocator = std::allocator<char>>
  class EntityTable
  {
  public:
    class Iterator;

    class Entity: public IEntity
    {
      friend class EntityTable;

      using string = std::basic_string<char, std::char_traits<char>, Allocator>;

      implement_lifetime_stub

    public:
      Entity( Allocator );
      Entity( std::string_view, uint32_t, Allocator );

    public:
      auto  SetOwnerPtr( mtc::api<Iface> p ) -> Entity*;

    public:   // overridables from IEntity
      auto  GetId() const -> Attribute override {  return { id, owner_ptr };  }
      auto  GetIndex() const -> uint32_t override {  return index;  }
      auto  GetAttributes() const -> Attribute override {  return { attribute, owner_ptr };  }
      auto  GetImage() const -> mtc::api<const mtc::IByteBuffer> override {  return {};  }

    public:   // serialization
      template <class O>
      O*  Serialize( O* ) const;

    protected:
      string                id;                   // public entity id
      string                attribute;            // document attributes
      uint32_t              index;                // order of creation, default 0

      mtc::api<Iface>       owner_ptr;
      std::atomic<Entity*>  collision = nullptr;  // relocation in the hash table
    };

  public:
    EntityTable( uint32_t size_limit, Allocator alloc = Allocator() );
   ~EntityTable();

    auto  GetMaxEntities() const -> size_t      {  return entStore.size();  }
    auto  GetHashTableSize() const -> size_t    {  return entTable.size();  }

    auto  GetEntityCount() const -> size_t      {  return ptrStore.load() - (Entity*)entStore.data() - 1;  }

  public:
  /*
   *  ::GetEntity( ... )
   *  Get entity by entity index or id.
   *  Returns entity-api or nullptr if not found or deleted.
   */
    auto  GetEntity( uint32_t id ) -> mtc::api<Entity>  {  return getEntity( *this, id );  }
    auto  GetEntity( uint32_t id ) const -> mtc::api<const Entity>  {  return getEntity( *this, id );  }
    auto  GetEntity( std::string_view id ) -> mtc::api<Entity>  {  return getEntity( *this, id );  }
    auto  GetEntity( std::string_view id ) const -> mtc::api<const Entity>  {  return getEntity( *this, id );  }

  /*
   *  ::DelEntity( string_view )
   *  Delete entity by entity id.
   *  Returns false if not found.
   */
    auto  DelEntity( std::string_view ) -> uint32_t;

  /*
   *  ::SetEntity( string_view, uint32_t* deleted )
   *  Set new entity with new object id, replace if exists.
   *  Returns api and stores replaced id.
   *
   *  Throws count_overflow if more than accepted count of documents.
   */
    auto  SetEntity( std::string_view, uint32_t* = nullptr ) -> mtc::api<Entity>;

  public:      // iterator access
    auto  GetIterator( uint32_t ) const -> Iterator;
    auto  GetIterator( std::string_view ) const -> Iterator;

  public:       // serialization
    template <class O>
    O*  Serialize( O* o ) const;

  protected:
    template <class Table>
    using EntityOf = typename std::conditional<std::is_const<Table>::value, const Entity, Entity>::type;

    auto  next_by_ix( uint32_t id ) const -> uint32_t;
    auto  next_by_id( uint32_t id ) const -> uint32_t;

  // access helpers
    auto  getEntity( uint32_t index ) const -> const Entity&
      {  return *(const Entity*)(index + entStore.data());  }
    auto  getEntity( uint32_t index )       -> Entity&
      {  return *(      Entity*)(index + entStore.data());  }

  // get implementation
    template <class S>
    static  auto  getEntity( S&, uint32_t ) -> mtc::api<EntityOf<S>>;
    template <class S>
    static  auto  getEntity( S&, std::string_view ) -> mtc::api<EntityOf<S>>;

  protected:
    using EntityHolder = typename std::aligned_storage<sizeof(Entity), alignof(Entity)>::type;
    using AtomicEntity = std::atomic<Entity*>;
    using EntityVector = std::vector<EntityHolder, Allocator>;
    using StrHashTable = std::vector<AtomicEntity, Allocator>;

    const size_t EntityHolderSize = sizeof(EntityHolder);

    EntityVector   entStore;  // the entities storage
    AtomicEntity   ptrStore;
    StrHashTable   entTable;

  };

  template <class Allocator>
  class EntityTable<Allocator>::Iterator
  {
    friend class EntityTable;

    using Step = uint32_t  (EntityTable::*)( uint32_t ) const;

    const EntityTable*  entityTable = nullptr;
    uint32_t            entityIndex = uint32_t(-1);

    Step                goToNextOne = nullptr;

  protected:
    Iterator( const EntityTable& et, uint32_t ix, Step np ):
      entityTable( &et ),
      entityIndex( ix ),
      goToNextOne( np ) {}

  public:
    Iterator() = default;
    Iterator( const Iterator& ) = default;

  public:
    auto  Curr() -> mtc::api<const Entity>;
    auto  Next() -> mtc::api<const Entity>;

  };

  // EntityTable::Entiry implementation

  template <class Allocator>
  EntityTable<Allocator>::Entity::Entity( Allocator mm ):
    id( mm ),
    attribute( mm ),
    index( 0 )  {}

  template <class Allocator>
  EntityTable<Allocator>::Entity::Entity( std::string_view id, uint32_t ix, Allocator mm ):
    id( id, mm ),
    attribute( mm ),
    index( ix ) {}

  template <class Allocator>
  template <class O>
  O*  EntityTable<Allocator>::Entity::Serialize( O* o ) const
  {
    return ::Serialize(
           ::Serialize(
           ::Serialize( o, index ), id ), attribute );
  }

  // EntityTable implementation

  template <class Allocator>
  EntityTable<Allocator>::EntityTable( uint32_t size_limit, Allocator alloc ):
    entStore( size_limit, alloc ),
    ptrStore( &getEntity( 1 ) ),
    entTable( UpperPrime( size_limit ), alloc )
  {
    new( entStore.data() )
      Entity( "", uint32_t(-1), alloc );
  }

  template <class Allocator>
  EntityTable<Allocator>::~EntityTable()
  {
    for ( auto beg = &getEntity( 0 ), end = ptrStore.load(); beg != end; ++beg )
      beg->~Entity();
  }


 /*
  *  EntityTable::DelEntity( string_view id )
  *
  *  Marks the document with specified id as deleted and removes from the linked list
  *  if found. Returns false if document is not found.
  */
  template <class Allocator>
  auto  EntityTable<Allocator>::DelEntity( std::string_view id ) -> uint32_t
  {
    auto  hindex = std::hash<decltype(id)>{}( id ) % entTable.size();
    auto* hentry = &entTable[hindex];
    auto  hvalue = mtc::ptr::clean( hentry->load() );
    auto  del_id = uint32_t(-1);

    // block the hash table entry from modifications
    while ( !hentry->compare_exchange_weak( hvalue, mtc::ptr::dirty( hvalue ) ) )
      hvalue = mtc::ptr::clean( hvalue );

    // search existing documents with same document id and remove
    for ( auto bfirst = true; hvalue != nullptr; hvalue = mtc::ptr::clean( hentry->load() ) )
      if ( hvalue->id == id )
      {
        if ( bfirst ) hentry->store( mtc::ptr::dirty( hvalue->collision.load() ) );
          else hentry->store( hvalue->collision.load() );

        del_id = hvalue - (Entity*)entTable.data();
        hvalue->index = uint32_t(-1);
      }
        else
      {
        hentry = &hvalue->collision;
        bfirst = false;
      }

  // unblock the entry
    entTable[hindex].store( mtc::ptr::clean( entTable[hindex].load() ) );
    return del_id;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::SetEntity( std::string_view id, uint32_t* deleted ) -> mtc::api<Entity>
  {
    auto  hindex = std::hash<decltype(id)>{}( id ) % entTable.size();
    auto* hentry = &entTable[hindex];
    auto  hvalue = mtc::ptr::clean( hentry->load() );
    auto  entptr = ptrStore.load();

    if ( id.empty() )
      throw std::invalid_argument( "id is empty" );

    if ( deleted != nullptr )
      *deleted = uint32_t(-1);

  // ensure allocate space for the new document;
  // throw exception on unmodified table if could not allocate;
  // finish with entptr -> allocated entry
    for ( ;; )
    {
      if ( entptr >= &getEntity( 0 ) + entStore.size() )
        throw count_overflow( mtc::strprintf( "index size achieved limit of %d documents", entStore.size() ) );

      if ( !ptrStore.compare_exchange_weak( entptr, entptr + 1 ) )
        continue;

      new( entptr ) Entity( id, entptr - &getEntity( 0 ), entTable.get_allocator() );
        break;
    }

  // Ok, the entity is allocated and no changes made to document set and relocation table;
  // ensure the element may be created with docid == -1 meaning it is 'deleted'
  //
  // Now block the hash table entry from modifications outside and create record to document
    while ( !hentry->compare_exchange_weak( hvalue, mtc::ptr::dirty( hvalue ) ) )
      hvalue = mtc::ptr::clean( hvalue );

  // search existing documents with same document id and remove
    for ( auto bfirst = true; hvalue != nullptr; hvalue = mtc::ptr::clean( hentry->load() ) )
      if ( hvalue->index != uint32_t(-1) && hvalue->id == id )
      {
      // exclude matching document from the list
        if ( bfirst ) hentry->store( mtc::ptr::dirty( hvalue->collision.load() ) );
          else hentry->store( hvalue->collision.load() );

      // mark excluded document as deleted
        hvalue->index = uint32_t(-1);

      // check if deleted document index is requested
        if ( deleted != nullptr )
          *deleted = hvalue - &getEntity( 0 );

        break;
      }
        else
      {
        hentry = &hvalue->collision;
        bfirst = false;
      }

    // set up the document chain
    entptr->collision.store( hvalue );
    entTable[hindex].store( entptr );

    return entptr;
  }

  /*
   *  EntityTable::getEntity( uint32_t index )
   *
   *  Scans the list element by element, looking for matching document index.
   */
  template <class Allocator>
  template <class S>
  auto  EntityTable<Allocator>::getEntity( S& self, uint32_t index ) -> mtc::api<EntityOf<S>>
  {
    auto  entptr = decltype(&self.getEntity( index )){};

    if ( index == 0 || index == uint32_t(-1) || index >= self.entStore.size() )
      throw std::invalid_argument( "index out of range" );

    if ( (entptr = &self.getEntity( index )) >= self.ptrStore.load() )
      return nullptr;

    return entptr->index != uint32_t(-1) ? entptr : nullptr;
  }

  template <class Allocator>
  template <class S>
  auto  EntityTable<Allocator>::getEntity( S& self, std::string_view id ) -> mtc::api<EntityOf<S>>
  {
    auto  hindex = std::hash<decltype(id)>{}( id ) % self.entTable.size();
    auto  docptr = mtc::ptr::clean( self.entTable[hindex].load() );

    // search matching document
    while ( docptr != nullptr && docptr->id != id )
      docptr = mtc::ptr::clean( docptr->collision.load() );

    // if found, lock and check if not deleted
    return docptr != nullptr && docptr->index != uint32_t(-1) ? docptr : nullptr;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::GetIterator( uint32_t id ) const -> Iterator
  {
    auto  beg = &getEntity( id );
    auto  end = ptrStore.load();

    while ( beg < end && beg->index == uint32_t(-1) )
      ++beg;

    return Iterator( *this, beg - &getEntity( 0 ), &EntityTable::next_by_ix );
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::GetIterator( std::string_view id ) const -> Iterator
  {
    auto  minone = (const Entity*)nullptr;

  // locate minimal entity with id >= passed one
    for ( auto beg = &getEntity( 1 ), end = ptrStore.load(); beg != end; ++beg )
      if ( beg->index != uint32_t(-1) && beg->id >= id )
        if ( minone == nullptr || beg->id < minone->id )
          minone = beg;

    return Iterator( *this, minone != nullptr ? minone - &getEntity( 0 ) : uint32_t(-1),
      &EntityTable::next_by_id );
  }

  template <class Allocator>
  template <class O>
  O*  EntityTable<Allocator>::Serialize( O* o ) const
  {
    std::vector<const Entity*>  sorted;

    if ( (o = Entity( entTable.get_allocator() ).Serialize( o )) == nullptr )
      return nullptr;

    sorted.reserve( ptrStore.load() - &getEntity( 1 ) );

    for ( auto ptr = &getEntity( 1 ), end = ptrStore.load(); ptr !=  end; ++ptr )
      if ( ptr->index != (uint32_t)-1 )
        sorted.emplace_back( ptr );

    std::sort( sorted.begin(), sorted.end(), []( const Entity* lhs, const Entity* rhs )
      {  return lhs->id < rhs->id; } );

    for ( auto& p: sorted )
      o = p->Serialize( o );

    return o;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::next_by_ix( uint32_t id ) const -> uint32_t
  {
    auto  ptrbeg = &getEntity( 0 );
    auto  ptrend = ptrStore.load();

    for ( ++id; ptrbeg + id < ptrend && ptrbeg[id].index == uint32_t(-1); ++id )
      (void)NULL;

    return ptrbeg + id < ptrend ? id : uint32_t(-1);
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::next_by_id( uint32_t id ) const -> uint32_t
  {
    auto  lastid = &getEntity( id ).id;
    auto  select = (const Entity*)nullptr;

    for ( auto beg = &getEntity( 1 ), end = ptrStore.load(); beg != end; ++beg )
      if ( beg->index != uint32_t(-1) && beg->id > *lastid )
        if ( select == nullptr || beg->id < select->id )
          lastid = &(select = beg)->id;

    return select != nullptr ? select->index : uint32_t(-1);
  }

  // EntityTable::Iterator implementation

  template <class Allocator>
  auto  EntityTable<Allocator>::Iterator::Curr() -> mtc::api<const Entity>
  {
    auto  maxIndex = entityTable->ptrStore.load() - &entityTable->getEntity( 0 );

    while ( entityIndex < maxIndex && entityTable->getEntity( entityIndex ).index == uint32_t(-1) )
      ++entityIndex;

    return entityIndex < maxIndex ? &entityTable->getEntity( entityIndex ) : nullptr;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::Iterator::Next() -> mtc::api<const Entity>
  {
    entityIndex = (entityTable->*goToNextOne)( entityIndex );

    return entityIndex != uint32_t(-1) ? &entityTable->getEntity( entityIndex ) : nullptr;
  }

}}}

# endif   // __palmira_src_index_dynamic_entities_hxx__
