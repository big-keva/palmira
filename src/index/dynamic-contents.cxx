# include "../api/dynamic-contents.hxx"
# include "dynamic-entities.hxx"
# include "dynamic-chains.hxx"
# include <mtc/arena.hpp>

namespace palmira {
namespace index {
namespace dynamic {

  class ContentsIndex final: public IContentsIndex
  {
    using Allocator = mtc::Arena::allocator<char>;

    using EntTable = EntityTable<Allocator>;
    using Contents = BlockChains<Allocator>;

    implement_lifetime_control

    class KeyValue;
    class Entities;
    class Iterator;

  public:
    ContentsIndex(
      uint32_t maxEntities,
      uint32_t maxAllocate, mtc::api<IStorage::IIndexStore> outputStorage );
   ~ContentsIndex() {  contents.StopIt();  }

  public:
    auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> override;

    bool  DelEntity( EntityId ) override;
    auto  SetEntity( EntityId,
      mtc::api<const IContents>         props = nullptr,
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr ) -> mtc::api<const IEntity> override;

    auto  GetMaxIndex() const -> uint32_t override  {  return entities.GetEntityCount();  }
    auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override;

    auto  GetIterator( EntityId ) -> mtc::api<IIterator> override;
    auto  GetIterator( uint32_t ) -> mtc::api<IIterator> override;

    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override  {  return this;  }

    void  Stash( EntityId ) override  {  throw std::logic_error("not implemented");  }

  protected:
    const uint32_t                  memLimit;
    mtc::Arena                      memArena;

    mtc::api<IStorage::IIndexStore> pStorage;

    EntTable                        entities;
    Contents                        contents;
    Bitmap<Allocator>               shadowed;

  };

  class ContentsIndex::KeyValue: public IIndexAPI
  {
    Contents& contents;
    uint32_t  entityId;

  public:
    KeyValue( Contents& cts, uint32_t ent ):
      contents( cts ),
      entityId( ent ) {}
    auto  ptr() const -> IIndexAPI* {  return (IIndexAPI*)this;  }

  public:
    void  Insert( const Span& key, const Span& value, unsigned bkType ) override
      {  return contents.Insert( key, entityId, value, bkType );  }

  };

  class ContentsIndex::Entities final: public IEntities
  {
    friend class ContentsIndex;

    using ChainHook = std::remove_pointer<decltype((
      ContentsIndex::contents.Lookup({})))>::type;
    using ChainLink = std::remove_pointer<decltype((
      ContentsIndex::contents.Lookup({})->pfirst.load()))>::type;

    implement_lifetime_control

  protected:
    Entities( ChainHook* chain, const ContentsIndex* owner ):
      pwhere( chain ),
      pchain( mtc::ptr::clean( chain->pfirst.load() ) ),
      parent( owner ) {}

  public:     // IEntities overridables
    auto  Find( uint32_t ) -> Reference override;
    auto  Type() const -> uint32_t override {  return pwhere->bkType;  }
    auto  Size() const -> uint32_t override {  return pwhere->ncount.load();  }

  protected:
    ChainHook*                    pwhere;
    ChainLink*                    pchain;
    mtc::api<const ContentsIndex> parent;

  };

  class ContentsIndex::Iterator final: public IIterator
  {
    implement_lifetime_control

  public:
    Iterator( EntTable::Iterator&& it, ContentsIndex* pc ):
      contents( pc ),
      iterator( std::move( it ) ) {}

  public:
    auto  Curr() -> mtc::api<const IEntity> override  {  return iterator.Curr().ptr();  }
    auto  Next() -> mtc::api<const IEntity> override  {  return iterator.Next().ptr();  }

  protected:
    mtc::api<ContentsIndex> contents;
    EntTable::Iterator      iterator;

  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( uint32_t maxEntities, uint32_t maxAllocate, mtc::api<IStorage::IIndexStore> storageSink  ):
    memLimit( maxAllocate ),
    pStorage( storageSink ),
    entities( maxEntities, this, memArena.get_allocator<char>() ),
    contents( memArena.get_allocator<char>() ),
    shadowed( maxEntities, memArena.get_allocator<char>() )
  {
  }

  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    return entities.GetEntity( id ).ptr();
  }

  auto  ContentsIndex::GetEntity( uint32_t id ) const -> mtc::api<const IEntity>
  {
    return entities.GetEntity( id ).ptr();
  }

  bool  ContentsIndex::DelEntity( EntityId id )
  {
    uint32_t  del_id = entities.DelEntity( id );

    return del_id != (uint32_t)-1 ? shadowed.Set( del_id ), true : false;
  }

  auto  ContentsIndex::SetEntity( EntityId id,
    mtc::api<const IContents>         props,
    mtc::api<const mtc::IByteBuffer>  /*attrs*/ ) -> mtc::api<const IEntity>
  {
    auto  entity = mtc::api<EntTable::Entity>();
    auto  del_id = uint32_t{};

  // check memory requirements
    if ( memArena.memusage() > memLimit )
      throw index_overflow( "dynamic index memory overflow" );

  // create the entity
    entity = entities.SetEntity( id, &del_id );

  // check if any entities deleted
    if ( del_id != uint32_t(-1) )
      shadowed.Set( del_id );

  // process contents indexing
    if ( props != nullptr )
      props->Enumerate( KeyValue( contents, entity->GetIndex() ).ptr() );

    return entity.ptr();
  }

  auto  ContentsIndex::GetKeyBlock( const void* key, size_t length ) const -> mtc::api<IEntities>
  {
    auto  pchain = contents.Lookup( { (const char*)key, length } );

    return pchain != nullptr && pchain->pfirst.load() != nullptr ?
      new Entities( pchain, this ) : nullptr;
  }

  auto  ContentsIndex::GetKeyStats( const void* key, size_t length ) const -> BlockInfo
  {
    auto  pchain = contents.Lookup( { (const char*)key, length } );

    if ( pchain != nullptr )
      return { pchain->bkType, pchain->ncount };
    return { uint32_t(-1), 0 };
  }

  auto  ContentsIndex::GetIterator( EntityId id ) -> mtc::api<IIterator>
  {
    return new Iterator( entities.GetIterator( id ), this );
  }

  auto  ContentsIndex::GetIterator( uint32_t ix ) -> mtc::api<IIterator>
  {
    return new Iterator( entities.GetIterator( ix ), this );
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
    if ( pStorage == nullptr )
      throw std::logic_error( "output storage is not defined, but FlushSink() was called" );

  // finalize keys thread and remove all the deleted elements from lists
    contents.StopIt().Remove( shadowed );

  // store entities table
    auto  tabStream = pStorage->Entities();
      entities.Serialize( tabStream.ptr() );

    auto  keyIndex = pStorage->Contents();
    auto  keyChain = pStorage->Chains();

    contents.Serialize( keyIndex.ptr(), keyChain.ptr() );

    return pStorage->Commit();
  }

  // ContentsIndex::Entities implemenation

  auto  ContentsIndex::Entities::Find( uint32_t id ) -> Reference
  {
    while ( pchain != nullptr && (pchain->entity < id || parent->shadowed.Get( pchain->entity )) )
      pchain = pchain->p_next.load();

    if ( pchain != nullptr )
      return { pchain->entity, { pchain->data(), pchain->lblock } };
    else
      return { uint32_t(-1), { nullptr, 0 } };
  }

  // contents implementation

  auto  Contents::Set( const Settings& options ) -> Contents&
    {  return openOptions = options, *this;  }

  auto  Contents::Set( mtc::api<IStorage::IIndexStore> storage ) -> Contents&
    {  return storageSink = storage, *this;  }

  auto  Contents::Create() const -> mtc::api<IContentsIndex>
    {  return new ContentsIndex( openOptions.maxEntities, openOptions.maxAllocate, storageSink );  }

}}}
