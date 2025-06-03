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

    class KeyValue: public IKeyValue
    {
      Contents& contents;
      uint32_t  entityId;

    public:
      KeyValue( Contents& cts, uint32_t ent ):
        contents( cts ),
        entityId( ent ) {}
      auto  ptr() const -> IKeyValue* {  return (IKeyValue*)this;  }

    public:
      void  Insert( std::string_view key, std::string_view value ) override
        {  return contents.Insert( key, entityId, value );  }

    };

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
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr,
      mtc::api<const mtc::IByteBuffer>  image = nullptr ) -> mtc::api<const IEntity> override;

    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override  {  return this;  }

    auto  GetMaxIndex() const -> uint32_t override  {  return entities.GetEntityCount();  }

  protected:
    const uint32_t                  memLimit;
    mtc::Arena                      memArena;

    mtc::api<IStorage::IIndexStore> pStorage;

    EntTable                        entities;
    Contents                        contents;
    BanList<Allocator>              skipList;

  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( uint32_t maxEntities, uint32_t maxAllocate, mtc::api<IStorage::IIndexStore> storageSink  ):
    memLimit( maxAllocate ),
    pStorage( storageSink ),
    entities( maxEntities, memArena.get_allocator<char>() ),
    contents( memArena.get_allocator<char>() ),
    skipList( maxEntities, memArena.get_allocator<char>() )
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

    return del_id != (uint32_t)-1 ? /* markup in banlist*/true : false;
  }

  auto  ContentsIndex::SetEntity( EntityId id,
    mtc::api<const IContents>         props,
    mtc::api<const mtc::IByteBuffer>  attrs,
    mtc::api<const mtc::IByteBuffer>  image ) -> mtc::api<const IEntity>
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
      skipList.Set( del_id );

  // process contents indexing
    if ( props != nullptr )
      props->Enumerate( KeyValue( contents, entity->GetIndex() ).ptr() );

    return entity.ptr();
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
    if ( pStorage == nullptr )
      throw std::logic_error( "output storage is not defined, but FlushSink() was called" );

  // finalize keys thread and remove all the deleted elements from lists
    contents
      .StopIt()/*.Remove( skipList )*/;

  // store entities table
    auto  tabStream = pStorage->Entities();
      entities.Serialize( tabStream.ptr() );

    auto  keyIndex = pStorage->Index();
    auto  keyChain = pStorage->Chains();

    contents.Serialize( keyIndex.ptr(), keyChain.ptr() );

    return pStorage->Commit();
  }

  // contents implementation

  auto  contents::SetAllocationLimit( uint32_t maxAllocateBytes ) -> contents&
    {  return maxAllocate = maxAllocateBytes, *this;  }
  auto  contents::SetMaxEntitiesCount( uint32_t maxEntitiesCount ) -> contents&
    {  return maxEntities = maxEntitiesCount, *this;  }
  auto  contents::SetOutStorageSink( mtc::api<IStorage::IIndexStore> sink ) -> contents&
    {  return storageSink = sink, *this;  }
  auto  contents::Create() const -> mtc::api<IContentsIndex>
    {  return new ContentsIndex( maxEntities, maxAllocate, storageSink );  }

}}}
