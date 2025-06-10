# include "../api/static-contents.hxx"
# include "static-entities.hxx"
# include "dynamic-bitmap.hxx"
# include <mtc/radix-tree.hpp>
# include <mtc/arena.hpp>

namespace palmira {
namespace index {

  class ContentsIndex final: public IContentsIndex
  {
    using Allocator = mtc::Arena::allocator<char>;
    using EntityTable = index::static_::EntityTable<Allocator>;
    using ISerialized = IStorage::ISerialized;
    using IByteBuffer = mtc::IByteBuffer;
    using IFlatStream = mtc::IFlatStream;
    using KeyContents = mtc::radix::dump<const char>;

    class EntitiesBase;
    class EntitiesLite;
    class EntitiesRich;

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

    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override  {  return this;  }

    auto  GetMaxIndex() const -> uint32_t override
      {  return entities.GetMaxEntities();  }

    auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override;

  protected:
    mtc::Arena                  memArena;       // allocation arena
    mtc::api<ISerialized>       xStorage;   // serialized object storage holder
    mtc::api<const IByteBuffer> tableBuf;
    mtc::api<const IByteBuffer> radixBuf;
    EntityTable                 entities;       // static entities table
    KeyContents                 contents;       // radix tree view
    mtc::api<IFlatStream>       blockBox;
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

  class ContentsIndex::EntitiesLite: public EntitiesBase
  {
    using EntitiesBase::EntitiesBase;

    implement_lifetime_control
  public:
    auto  Find( uint32_t ) -> Reference override;

  };

  class ContentsIndex::EntitiesRich: public EntitiesBase
  {
    using EntitiesBase::EntitiesBase;

    implement_lifetime_control
  public:
    auto  Find( uint32_t ) -> Reference override;

  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( mtc::api<IStorage::ISerialized> storage ):
    xStorage( storage ),
    tableBuf( storage->Entities() ),
    radixBuf( storage->Contents() ),
    entities( tableBuf, memArena.get_allocator<char>() ),
    contents( radixBuf->GetPtr() ),
    blockBox( storage->Blocks() ),
    shadowed( entities.GetMaxEntities(), memArena.get_allocator<char>() )
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

  bool  ContentsIndex::DelEntity( EntityId id )
  {
  }

  auto  ContentsIndex::SetEntity( EntityId,
    mtc::api<const IContents>, mtc::api<const mtc::IByteBuffer> ) -> mtc::api<const IEntity>
  {
    throw std::logic_error( "static_::ContentsIndex::SetEntity( ) must not be called" );
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
    return xStorage;
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
    return { 0, 0 };
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
    for ( tofind = std::max( tofind, 1U ); ptrtop != ptrend; )
    {
      unsigned  udelta;
      unsigned  ublock;

      if ( (ptrtop = ::FetchFrom( ::FetchFrom( ptrtop, udelta ), ublock )) == nullptr )
        return curref = { (uint32_t)-1, { nullptr, 0 } };

      if ( (curref.uEntity += udelta + 1) >= tofind )
      {
        curref.details = { ptrtop, ublock + 1 };
        return ptrtop += ublock + 1, curref;
      }

      ptrtop += ublock + 1;
    }
    return curref.uEntity >= tofind ? curref : curref = { (uint32_t)-1, { nullptr, 0 } };
  }

  // contents implementation

  auto  Static::Create( mtc::api<IStorage::ISerialized> serialized ) -> mtc::api<IContentsIndex>
  {
    if ( serialized->Entities() == nullptr )
      throw std::invalid_argument( "static_::ContentsIndex::Create() must not be called" );
    return new ContentsIndex( serialized );
  }

}}
