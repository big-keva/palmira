# include "index-flakes.hxx"
# include "override-entities.hxx"

namespace palmira {
namespace index {

  class IndexFlakes::Entities final: public IContentsIndex::IEntities
  {
    struct BlockEntry
    {
      uint32_t            uLower;
      uint32_t            uUpper;
      mtc::api<IEntities> entSet;
      const Bitmap<>&     banned;
    };

    using BlockSet = std::vector<BlockEntry>;

    mtc::api<const mtc::Iface>        holder;
    BlockSet                          blocks;
    mutable BlockSet::const_iterator  pblock;
    uint32_t                          ncount = 0;
    uint32_t                          bktype = uint32_t(-1);

    implement_lifetime_control

  public:
    Entities( const mtc::Iface* parent = nullptr );

    void  AddBlock( const BlockEntry& );

  public:
    auto  Find( uint32_t ) -> Reference override;
    auto  Size() const -> uint32_t override {  return ncount;  }
    auto  Type() const -> uint32_t override {  return bktype;  }

  };

  // IndexFlakes implementation

 /*
  * getEntity( id )
  *
  * Returns entity from one of indices held which is not excluded from by other indices
  * with override process.
  *
  * The resulted entity has overriden index.
  */
  auto  IndexFlakes::getEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    for ( auto beg = indices.rbegin(); beg != indices.rend(); ++beg )
    {
      auto  entity = beg->pIndex->GetEntity( id );
      auto  uindex = uint32_t{};

      if ( entity != nullptr && !beg->banned.Get( uindex = entity->GetIndex() ) )
        return beg->Override( entity );
    }
    return {};
  }

  auto  IndexFlakes::getEntity( uint32_t ix ) const -> mtc::api<const IEntity>
  {
    for ( auto& next: indices )
      if ( next.uLower <= ix && next.uUpper >= ix )
        if ( !next.banned.Get( ix - next.uLower + 1 ) )
        {
          auto  entity = next.pIndex->GetEntity( ix - next.uLower + 1 );

          if ( entity != nullptr )
            return next.uLower > 1 ? Override( entity ).Index( ix ) : entity;
        }
    return {};
  }

  bool  IndexFlakes::delEntity( EntityId id )
  {
    auto  deleted = false;

    for ( auto& next: indices )
      deleted |= next.pIndex->DelEntity( id );
    return deleted;
  }

  auto  IndexFlakes::getMaxIndex() const -> uint32_t
  {
    return indices.size() != 0 ? indices.back().uUpper : 0;
  }

  auto  IndexFlakes::getKeyBlock( const void* key, size_t len, const mtc::Iface* pix ) const -> mtc::api<IContentsIndex::IEntities>
  {
    mtc::api<Entities>  entities;

    for ( auto& next: indices )
    {
      auto  pblock = next.pIndex->GetKeyBlock( key, len );

      if ( pblock != nullptr )
      {
        if ( entities == nullptr )
          entities = new Entities( pix );

        entities->AddBlock( { next.uLower, next.uUpper, pblock, next.banned } );
      }
    }
    return entities.ptr();
  }

  auto  IndexFlakes::getKeyStats( const void* key, size_t len ) const -> IContentsIndex::BlockInfo
  {
    IContentsIndex::BlockInfo blockStats = { uint32_t(-1), 0 };

    for ( auto& next: indices )
    {
      auto  cStats = next.pIndex->GetKeyStats( key, len );

      if ( blockStats.bkType == uint32_t(-1) )  blockStats = cStats;
        else
      if ( blockStats.bkType == cStats.bkType ) blockStats.nCount += cStats.nCount;
        else
      throw std::invalid_argument( "Block types differ in sequental indives" );
    }

    return blockStats;
  }

  void  IndexFlakes::add( mtc::api<IContentsIndex> ix )
  {
    auto  uLower = indices.empty() ? 1 : indices.back().uUpper + 1;

    indices.emplace_back( uLower, ix );
  }

  // IndexFlakes::IndexEntry implementation

  IndexFlakes::IndexEntry::IndexEntry( uint32_t lower, mtc::api<IContentsIndex> index ):
    uLower( lower ),
    uUpper( uLower + index->GetMaxIndex() - 1 ),
    pIndex( index ),
    banned( uUpper - uLower + 1 )
  {
  }

  auto  IndexFlakes::IndexEntry::Override( mtc::api<const IEntity> entity ) const -> mtc::api<const IEntity>
  {
    return uLower > 1 ? index::Override( entity ).Index(
      entity->GetIndex() + uLower - 1 ) : entity;
  }

  // IndexFlakes::Entities implementation

  IndexFlakes::Entities::Entities( const mtc::Iface* pix ):
    holder( pix ), pblock( blocks.begin() )
  {
  }

  void  IndexFlakes::Entities::AddBlock( const BlockEntry& block )
  {
    if ( blocks.empty() )
      bktype = block.entSet->Type();

    blocks.emplace_back( block );
      pblock = blocks.begin();
      ncount += block.entSet->Size();
  }

  auto  IndexFlakes::Entities::Find( uint32_t ix ) -> Reference
  {
    for ( auto  getRef = Reference(); pblock != blocks.end(); ++pblock )
    {
      if ( ix < pblock->uLower )
        ix = pblock->uLower;

      while ( pblock->uUpper < ix )
        ++pblock;

      if ( pblock == blocks.end() )
        break;

      for ( auto findId = ix - pblock->uLower + 1; ; findId = getRef.uEntity + 1 )
      {
        if ( (getRef = pblock->entSet->Find( findId )).uEntity == (uint32_t)-1 )
          {  ix = pblock->uUpper + 1;  break;  }

        if ( !pblock->banned.Get( getRef.uEntity ) )
          return getRef.uEntity += pblock->uLower - 1, getRef;
      }
    }
    return { uint32_t(-1), {} };
  }

}}
