# if !defined( __palmira_src_index_static_entities_hxx__ )
# define __palmira_src_index_static_entities_hxx__
# include "../../api/contents-index.hxx"
# include "../tools/primes.hxx"
# include <mtc/ptrpatch.h>
# include <functional>
# include <stdexcept>
# include <algorithm>

namespace palmira {
namespace index   {
namespace static_ {

  template <class Allocator = std::allocator<char>>
  class EntityTable
  {
  public:
    class Entity: public IEntity
    {
      friend class EntityTable;

      using string = std::basic_string<char, std::char_traits<char>, Allocator>;

      implement_lifetime_stub

    public:
      Entity() = default;

    // overridables from IEntity
      auto  GetId() const -> Attribute override {  return { entity_id, owner };  }
      auto  GetIndex() const -> uint32_t override {  return index;  }
      auto  GetAttributes() const -> Attribute override {  return { attribute, owner };  }

      auto  SetOwnerId( mtc::Iface* pw ) -> Entity* {  return owner = pw, this;  }
      bool  ValidIndex() const noexcept {  return index != 0 && index != uint32_t(-1);  }

    protected:
    // loading
      auto  FetchFrom( const char* ) -> const char*;

    public:
      mtc::Iface*       owner = nullptr;
      Entity*           collision = nullptr;
      uint32_t          index = 0;            // order of creation, default 0
      std::string_view  entity_id;
      std::string_view  attribute;

    };

    class Iterator;

  protected:
    using vector_type = std::vector<Entity, Allocator>;
    using hash_vector = std::vector<Entity*, Allocator>;

  public:
    EntityTable( const Span&, Allocator = Allocator() );
   ~EntityTable();

    auto  GetMaxEntities() const -> size_t {  return entityTable.size() - 1;  };
    auto  GetEntityCount() const -> size_t {  return entityTable.size() - 1;  };

  // entities access
    auto  GetEntity( uint32_t id ) const -> mtc::api<const Entity>;
    auto  GetEntity( std::string_view id ) const -> mtc::api<const Entity>;

  // iterator
    auto  GetIterator( uint32_t ) const -> Iterator;
    auto  GetIterator( std::string_view ) const -> Iterator;

  protected:
    auto  getKeyIndex() const -> const EntityTable&;
    auto  getNextByIx( uint32_t id ) const -> uint32_t;
    auto  getNextById( uint32_t id ) const -> uint32_t;

  protected:
    using IndexByKeys = std::vector<uint32_t, Allocator>;

    vector_type                             entityTable;
    hash_vector                             entitiesMap;
    mutable std::atomic<IndexByKeys*>       indexByKeys = nullptr;

  };

  template <class Allocator>
  class EntityTable<Allocator>::Iterator
  {
    friend class EntityTable;

    using FnNext = uint32_t  (EntityTable::*)( uint32_t ) const;

    const EntityTable&  parent;
    FnNext              fnNext;
    uint32_t            uindex;

  protected:
    Iterator( const EntityTable& table, FnNext fnext, uint32_t index ):
      parent( table ),
      fnNext( fnext ),
      uindex( index ) {}

  public:     // construction
    Iterator( const Iterator& ) = default;
    Iterator& operator=( const Iterator& ) = default;

  public:     // iterator properties
    auto  Curr() -> const Entity*;
    auto  Next() -> const Entity*;

  };

  // EntityTable::Entity implementation

  template <class Allocator>
  auto  EntityTable<Allocator>::Entity::FetchFrom( const char* src ) -> const char*
  {
    unsigned  cch;

  // get index and id length; set entity id
    if ( (src = ::FetchFrom( ::FetchFrom( src, index ), cch )) == nullptr )
      return nullptr;

    src = (entity_id = { src, cch }).data() + cch;

  // get attribute length
    if ( (src = ::FetchFrom( src, cch )) == nullptr )
      return nullptr;

    return src = (attribute = { src, cch }).data() + cch;
  }

  // EntityTable implementation

  template <class Allocator>
  EntityTable<Allocator>::EntityTable( const Span& input, Allocator alloc ):
    entityTable( alloc ),
    entitiesMap( alloc )
  {
    entityTable.reserve( input.size() / 0x40 );

  // load the table
    for ( auto src = input.data(), end = input.data() + input.size(); src != nullptr && src != end; )
    {
      entityTable.resize( entityTable.size() + 1 );
        src = entityTable.back().FetchFrom( src );
    }

  // allocate hash table
    entitiesMap.resize( UpperPrime( entityTable.size() ) );

    for ( auto& next: entityTable )
      if ( next.index != 0 && next.index != uint32_t(-1) )
      {
        auto  hash = std::hash<std::string_view>{}( next.entity_id );
        auto  offs = hash % entitiesMap.size();

        next.collision = entitiesMap[offs];
        entitiesMap[offs] = &next;
      }
  }

  template <class Allocator>
  EntityTable<Allocator>::~EntityTable()
  {
    auto   pindex = mtc::ptr::clean( indexByKeys.load() );

    while ( pindex != nullptr && !indexByKeys.compare_exchange_weak( pindex, mtc::ptr::dirty( pindex ) ) )
      pindex = mtc::ptr::clean( pindex );

    if ( pindex != nullptr )
      delete pindex;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::GetEntity( uint32_t id ) const -> mtc::api<const Entity>
  {
    return id > 0 && id < entityTable.size() ? &entityTable[id] : nullptr;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::GetEntity( std::string_view id ) const -> mtc::api<const Entity>
  {
    if ( id.empty() )
      throw std::invalid_argument( "empty entity id" );

    if ( entitiesMap.size() != 0 )
    {
      auto  hash = std::hash<std::string_view>{}( id );
      auto  offs = hash % entitiesMap.size();

      for ( auto next = entitiesMap[offs]; next != nullptr; next = next->collision )
        if ( next->entity_id == id )
          return next;
    }

    return nullptr;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::GetIterator( uint32_t id ) const -> Iterator
  {
    for ( ; id < entityTable.size(); ++id )
    {
      if ( entityTable[id].ValidIndex() )
        return Iterator( *this, &EntityTable::getNextByIx, id );
    }
    return Iterator( *this, &EntityTable::getNextByIx, uint32_t(-1) );
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::GetIterator( std::string_view id ) const -> Iterator
  {
    auto  pindex = getKeyIndex().indexByKeys.load();
    auto  pfound = std::lower_bound( pindex->begin(), pindex->end(), id, [&]( uint32_t i, std::string_view id )
      {  return entityTable[i].entity_id <= id;  } );

    while ( pfound != pindex->end() && !entityTable[*pfound].ValidIndex() )
      ++pfound;

    return Iterator( *this, &EntityTable::getNextById, pfound != pindex->end() ?
      *pfound : uint32_t(-1) );
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::getKeyIndex() const -> const EntityTable&
  {
    auto  pindex = mtc::ptr::clean( indexByKeys.load() );

    for ( ; ; )
    {
      if ( pindex != nullptr )
        return *this;

      if ( indexByKeys.compare_exchange_weak( pindex, mtc::ptr::dirty( pindex ) ) )
        if ( (pindex = mtc::ptr::clean( pindex )) == nullptr )
        {
          auto  malloc = typename std::allocator_traits<Allocator>::rebind_alloc<IndexByKeys>(
            entityTable.get_allocator() );

        // allocate new table
          pindex = new( malloc.allocate( 1 ) )
            IndexByKeys( malloc );
          pindex->reserve( entityTable.size() - 1 );

        // fill table with document indices
          for ( auto& next: entityTable )
            if ( next.index != 0 && next.index != uint32_t(-1) )
              pindex->push_back( next.index );

        // sort ids by entity id
          std::sort( pindex->begin(), pindex->end(), [&]( uint32_t lhs, uint32_t rhs )
            {  return entityTable[lhs].entity_id < entityTable[rhs].entity_id;  } );

          return indexByKeys = pindex, *this;
        }
    }
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::getNextByIx( uint32_t id ) const -> uint32_t
  {
    if ( id != uint32_t(-1) )
    {
      for ( ++id; id != entityTable.size() && !entityTable[id].ValidIndex(); ++id )
        (void)NULL;
      if ( id == entityTable.size() )
        id = uint32_t(-1);
    }
    return id;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::getNextById( uint32_t id ) const -> uint32_t
  {
    if ( id != uint32_t(-1) )
    {
      auto  pindex = indexByKeys.load();
      auto  pfound = std::find( pindex->begin(), pindex->end(), id );

      if ( pfound != pindex->end() )
        ++pfound;

      while ( pfound != pindex->end() && !entityTable[*pfound].ValidIndex() )
        ++pfound;

      if ( pfound != pindex->end() )
        return *pfound;
    }
    return uint32_t(-1);
  }

  // EntityTable::Iterator implementation

  template <class Allocator>
  auto  EntityTable<Allocator>::Iterator::Curr() -> const Entity*
  {
    return uindex != uint32_t(-1) ? &parent.entityTable[uindex] : nullptr;
  }

  template <class Allocator>
  auto  EntityTable<Allocator>::Iterator::Next() -> const Entity*
  {
    if ( uindex != uint32_t(-1) )
    {
      if ( (uindex = (parent.*fnNext)( uindex )) != uint32_t(-1) )
        return &parent.entityTable[uindex];
    }
    return nullptr;
  }

}}}

# endif   // __palmira_src_index_static_entities_hxx__
