# include "../api/static-contents.hxx"
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
    using KeyContents = mtc::radix::dump<const char>;
    using PatchHolder = PatchTable<Allocator>;

    class EntitiesBase;
    class EntitiesLite;
    class EntitiesRich;
    class Iterator;

    implement_lifetime_control

  public:
    ContentsIndex( mtc::api<IStorage::ISerialized> storage );
   ~ContentsIndex() = default;

  public:
    auto  GetEntity( EntityId id ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t id ) const -> mtc::api<const IEntity> override;
    bool  DelEntity( EntityId ) override;
    auto  SetEntity( EntityId,
      mtc::api<const IContents>   props = nullptr,
      mtc::api<const IByteBuffer> attrs = nullptr ) -> mtc::api<const IEntity> override;

    auto  GetMaxIndex() const -> uint32_t override
      {  return entities.GetEntityCount();  }

    auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override;

    auto  GetIterator( EntityId ) -> mtc::api<IIterator> override;
    auto  GetIterator( uint32_t ) -> mtc::api<IIterator> override;

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
    KeyContents                 contents;       // radix tree view
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

  class ContentsIndex::Iterator final: public IIterator
  {
    implement_lifetime_control

  public:
    Iterator( EntityTable::Iterator&& it, ContentsIndex* pc ):
      contents( pc ),
      iterator( std::move( it ) ) {}

  public:
    auto  Curr() -> mtc::api<const IEntity> override;
    auto  Next() -> mtc::api<const IEntity> override;

  protected:
    mtc::api<ContentsIndex> contents;
    EntityTable::Iterator   iterator;

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

    return entity != nullptr && !shadowed.Get( entity->index ) ? entity.ptr() : nullptr;
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

  auto  ContentsIndex::SetEntity( EntityId,
    mtc::api<const IContents>, mtc::api<const mtc::IByteBuffer> ) -> mtc::api<const IEntity>
  {
    throw std::logic_error( "static_::ContentsIndex::SetEntity( ) must not be called" );
  }

  auto  ContentsIndex::GetKeyBlock( const void* key, size_t size ) const -> mtc::api<IEntities>
  {
    auto  pfound = contents.Search( { (const char*)key, size } );

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

  auto  ContentsIndex::GetKeyStats( const void* key, size_t size ) const -> BlockInfo
  {
    auto  pfound = contents.Search( { (const char*)key, size } );

    if ( pfound != nullptr )
    {
      BlockInfo blockInfo;

      if ( ::FetchFrom( ::FetchFrom( pfound, blockInfo.bkType ), blockInfo.nCount ) != nullptr )
        return blockInfo;
    }
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
    patchTab.Delete( { id.data(), id.size() } );
    patchTab.Delete( index );
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

  // ContentsIndex::Iterator implementation

  auto  ContentsIndex::Iterator::Curr() -> mtc::api<const IEntity>
  {
    for ( auto ent = iterator.Curr(); ent != nullptr; ent = iterator.Next() )
      if ( !contents->shadowed.Get( ent->GetIndex() ) )
        return ent;

    return nullptr;
  }

  auto  ContentsIndex::Iterator::Next() -> mtc::api<const IEntity>
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
