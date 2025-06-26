# include "indices/static-contents.hpp"
# include "override-entities.hxx"
# include "static-entities.hxx"
# include "dynamic-bitmap.hxx"
# include "patch-table.hxx"
# include <mtc/radix-tree.hpp>
# include <mtc/arena.hpp>

namespace palmira {
namespace index {
namespace static_ {

  class ContentsIndex final: public IContentsIndex
  {
    using Allocator = mtc::Arena::allocator<char>;
    using EntityTable = index::static_::EntityTable<Allocator>;
    using ISerialized = IStorage::ISerialized;
    using IByteBuffer = mtc::IByteBuffer;
    using IFlatStream = mtc::IFlatStream;
    using PatchHolder = PatchTable<Allocator>;
    using ContentsTable = mtc::radix::dump<const char>;
    using ContentsIterator = ContentsIndex::ContentsTable::const_iterator;

    class EntitiesBase;
    class EntitiesLite;
    class EntitiesRich;
    class EntityIterator;
    class RecordIterator;

    implement_lifetime_control

  public:
    ContentsIndex( mtc::api<IStorage::ISerialized> storage );
   ~ContentsIndex() = default;

  public:
    auto  GetEntity( EntityId id ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t id ) const -> mtc::api<const IEntity> override;
    bool  DelEntity( EntityId ) override;
    auto  SetEntity( EntityId, mtc::api<const IContents>, const Span& ) -> mtc::api<const IEntity> override;
    auto  SetExtras( EntityId, const Span& ) -> mtc::api<const IEntity> override;

    auto  GetMaxIndex() const -> uint32_t override
      {  return entities.GetEntityCount();  }

    auto  GetKeyBlock( const Span& ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const Span& ) const -> BlockInfo override;

    auto  GetEntityIterator( EntityId ) -> mtc::api<IEntityIterator> override;
    auto  GetEntityIterator( uint32_t ) -> mtc::api<IEntityIterator> override;

    auto  GetRecordIterator( const Span& ) -> mtc::api<IRecordIterator> override;

    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override  {  return this;  }

    void  Stash( EntityId ) override;

  protected:
    bool  delEntity( EntityId, uint32_t );

  protected:
    mtc::Arena                  memArena;       // allocation arena
    mtc::api<ISerialized>       xStorage;   // serialized object storage holder
    mtc::api<const IByteBuffer> tableBuf;
    mtc::api<const IByteBuffer> radixBuf;
    EntityTable                 entities;       // static entities table
    ContentsTable               contents;       // radix tree view
    mtc::api<IFlatStream>       blockBox;
    PatchHolder                 patchTab;
    Bitmap<Allocator>           shadowed;       // deleted documents identifiers

  };

  class ContentsIndex::EntitiesBase: public IEntities
  {
  public:
    EntitiesBase( const mtc::api<const mtc::IByteBuffer>&, uint32_t, uint32_t, const ContentsIndex* );

  public:     // overridables
    auto  Size() const -> uint32_t override {  return ncount;  }
    auto  Type() const -> uint32_t override {  return bkType;  }

  protected:
    const uint32_t                    bkType;
    const uint32_t                    ncount;
    mtc::api<const ContentsIndex>     parent;
    mtc::api<const mtc::IByteBuffer>  iblock;
    const char*                       ptrtop;
    const char*                       ptrend;
    Reference                         curref = { 0, { nullptr, 0 } };

  };

  class ContentsIndex::EntitiesLite final: public EntitiesBase
  {
    using EntitiesBase::EntitiesBase;

    implement_lifetime_control
  public:
    auto  Find( uint32_t ) -> Reference override;

  };

  class ContentsIndex::EntitiesRich final: public EntitiesBase
  {
    using EntitiesBase::EntitiesBase;

    implement_lifetime_control
  public:
    auto  Find( uint32_t ) -> Reference override;

  };

  class ContentsIndex::EntityIterator final: public IEntityIterator
  {
    implement_lifetime_control

  public:
    EntityIterator( EntityTable::Iterator&& it, ContentsIndex* pc ):
      contents( pc ),
      iterator( std::move( it ) ) {}

  public:
    auto  Curr() -> mtc::api<const IEntity> override;
    auto  Next() -> mtc::api<const IEntity> override;

  protected:
    mtc::api<ContentsIndex> contents;
    EntityTable::Iterator   iterator;

  };

  class ContentsIndex::RecordIterator final: public IRecordIterator
  {
    implement_lifetime_control

  public:
    RecordIterator( ContentsIndex* pc ):
      contents( pc ),
      iterator( pc->contents.begin() ){}

  public:
    auto  Curr() -> std::string override
    {
        return iterator != contents->contents.end() ? iterator->key.to_string() : "";
    }
    auto  Next() -> std::string override
    {
      return iterator != contents->contents.end() && ++iterator != contents->contents.end() ?
        iterator->key.to_string() : "";
    }

  protected:
    mtc::api<ContentsIndex> contents;
    ContentsIterator        iterator;

  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( mtc::api<IStorage::ISerialized> storage ):
    xStorage( storage ),
    tableBuf( storage->Entities() ),
    radixBuf( storage->Contents() ),
    entities( tableBuf, this, memArena.get_allocator<char>() ),
    contents( radixBuf->GetPtr() ),
    blockBox( storage->Blocks() ),
    patchTab( std::max( 1000U, entities.GetEntityCount() ), memArena.get_allocator<char>() ),
    shadowed( entities.GetEntityCount(), memArena.get_allocator<char>() )
  {
  }

  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    auto  entity = entities.GetEntity( id );

    if ( entity != nullptr && !shadowed.Get( entity->index ) )
    {
      auto  ppatch = patchTab.Search( { id.data(), id.size() } );

      if ( ppatch == nullptr )
        return entity.ptr();

      if ( ppatch->GetLen() == size_t(-1) )
        return nullptr;

      return Override::Entity( entity.ptr() ).Extra( ppatch );
    }
    return nullptr;
  }

  auto  ContentsIndex::GetEntity( uint32_t id ) const -> mtc::api<const IEntity>
  {
    return !shadowed.Get( id ) ? entities.GetEntity( id ).ptr() : nullptr;
  }

 /*
  * DelEntity( EntityId id )
  *
  * Creates the patch record for the deleted document.
  */
  bool  ContentsIndex::DelEntity( EntityId id )
  {
    auto  getdoc = entities.GetEntity( id );

    if ( getdoc != nullptr )
    {
      auto  ppatch = patchTab.Search( { id.data(), id.size() } );

      if ( ppatch == nullptr || ppatch->GetLen() != size_t(-1) )
        return delEntity( id, getdoc->index );
    }
    return false;
  }

  auto  ContentsIndex::SetEntity( EntityId, mtc::api<const IContents>, const Span& ) -> mtc::api<const IEntity>
  {
    throw std::logic_error( "static_::ContentsIndex::SetEntity( ) must not be called" );
  }

  auto  ContentsIndex::SetExtras( EntityId id, const Span& xtras ) -> mtc::api<const IEntity>
  {
    auto  getdoc = entities.GetEntity( id );

    if ( getdoc != nullptr && !shadowed.Get( getdoc->index ) )
    {
      auto  ppatch = patchTab.Update( { id.data(), id.size() }, getdoc->index, xtras );

      return ppatch == nullptr || ppatch->GetLen() != size_t(-1) ?
        Override::Entity( getdoc.ptr() ).Extra( ppatch ) : getdoc.ptr();
    }
    return nullptr;
  }

  auto  ContentsIndex::GetKeyBlock( const Span& key ) const -> mtc::api<IEntities>
  {
    auto  pfound = contents.Search( { key.data(), key.size() } );

    if ( pfound != nullptr )
    {
      uint32_t  blockType;
      uint32_t  nEntities;
      uint64_t  blockOffs;
      uint32_t  blockSize;

      if ( ::FetchFrom( ::FetchFrom( ::FetchFrom( ::FetchFrom( pfound,
        blockType ),
        nEntities ),
        blockOffs ),
        blockSize ) != nullptr )
      {
        auto  pblock = mtc::api<const IByteBuffer>( blockBox->PGet( blockOffs, blockSize ).ptr() );

        if ( blockType == 0 )
          return new EntitiesLite( pblock, blockType, nEntities, this );
        else
          return new EntitiesRich( pblock, blockType, nEntities, this );
      }
    }
    return nullptr;
  }

  auto  ContentsIndex::GetKeyStats( const Span& key ) const -> BlockInfo
  {
    auto  pfound = contents.Search( { key.data(), key.size() } );

    if ( pfound != nullptr )
    {
      BlockInfo blockInfo;

      if ( ::FetchFrom( ::FetchFrom( pfound, blockInfo.bkType ), blockInfo.nCount ) != nullptr )
        return blockInfo;
    }
    return { uint32_t(-1), 0 };
  }

  auto  ContentsIndex::GetEntityIterator( EntityId id ) -> mtc::api<IEntityIterator>
  {
    return new EntityIterator( entities.GetIterator( id ), this );
  }

  auto  ContentsIndex::GetEntityIterator( uint32_t ix ) -> mtc::api<IEntityIterator>
  {
    return new EntityIterator( entities.GetIterator( ix ), this );
  }

  auto  ContentsIndex::GetRecordIterator( const Span& ) -> mtc::api<IRecordIterator>
  {
    return new RecordIterator( this );
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
    return xStorage;
  }

  void  ContentsIndex::Stash( EntityId id )
  {
    auto  getdoc = entities.GetEntity( id );

    if ( getdoc != nullptr )
      shadowed.Set( getdoc->index );
  }

  bool  ContentsIndex::delEntity( EntityId id, uint32_t index )
  {
    patchTab.Delete( { id.data(), id.size() }, index );
    shadowed.Set( index );
    return true;
  }

  // ContentsIndex::EntitiesBase implementation

  ContentsIndex::EntitiesBase::EntitiesBase(
    const mtc::api<const mtc::IByteBuffer>& src,
    uint32_t                                btp,
    uint32_t                                cnt,
    const ContentsIndex*                    own ):
      bkType( btp ),
      ncount( cnt ),
      parent( own ),
      iblock( src ),
      ptrtop( src->GetPtr() ),
      ptrend( ptrtop + src->GetLen() )
  {
  }

  // ContentsIndex::EntitiesLite implementation

  auto  ContentsIndex::EntitiesLite::Find( uint32_t tofind ) -> Reference
  {
    for ( tofind = std::max( tofind, 1U ); curref.uEntity < tofind; )
    {
      unsigned  udelta;

      if ( (ptrtop = ::FetchFrom( ptrtop, udelta )) == nullptr )
        return curref = { (uint32_t)-1, { nullptr, 0 } };
      curref.uEntity += udelta + 1;
    }
    return curref.uEntity >= tofind ? curref : curref = { (uint32_t)-1, { nullptr, 0 } };
  }

  // ContentsIndex::EntitiesRich implementation

  auto  ContentsIndex::EntitiesRich::Find( uint32_t tofind ) -> Reference
  {
    for ( tofind = std::max( tofind, 1U ); ptrtop < ptrend; )
    {
      unsigned  udelta;
      unsigned  ublock;

      if ( (ptrtop = ::FetchFrom( ::FetchFrom( ptrtop, udelta ), ublock )) == nullptr )
        return curref = { (uint32_t)-1, { nullptr, 0 } };

      if ( (curref.uEntity += udelta + 1) >= tofind && !parent->shadowed.Get( curref.uEntity ) )
      {
        curref.details = { ptrtop, ublock + 1 };
        return ptrtop += ublock + 1, curref;
      }

      if ( (ptrtop += ublock + 1) >= ptrend )
        break;
    }
    return curref = { (uint32_t)-1, { nullptr, 0 } };
  }

  // ContentsIndex::EntityIterator implementation

  auto  ContentsIndex::EntityIterator::Curr() -> mtc::api<const IEntity>
  {
    for ( auto ent = iterator.Curr(); ent != nullptr; ent = iterator.Next() )
      if ( !contents->shadowed.Get( ent->GetIndex() ) )
      {
        auto  patch = contents->patchTab.Search( ent->GetIndex() );

        return patch != nullptr ? Override::Entity( ent ).Extra( patch ) : ent;
      }

    return nullptr;
  }

  auto  ContentsIndex::EntityIterator::Next() -> mtc::api<const IEntity>
  {
    for ( auto ent = iterator.Next(); ent != nullptr; ent = iterator.Next() )
      if ( !contents->shadowed.Get( ent->GetIndex() ) )
        return ent;

    return nullptr;
  }

  // contents implementation

  auto  Contents::Create( mtc::api<IStorage::ISerialized> serialized ) -> mtc::api<IContentsIndex>
  {
    if ( serialized->Entities() == nullptr )
      throw std::invalid_argument( "static_::ContentsIndex::Create() must not be called" );
    return new ContentsIndex( serialized );
  }

}}}
