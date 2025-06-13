# include "../../api/dynamic-contents.hxx"
# include "../../api/static-contents.hxx"
# include "override-entities.hxx"
# include "dynamic-entities.hxx"
# include "commit-contents.hxx"
# include "dynamic-bitmap.hxx"
# include "index-flakes.hxx"
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/ptrpatch.h>


namespace palmira {
namespace index {
namespace flaked {

  class ContentsIndex final: public IContentsIndex, protected IndexFlakes
  {
    implement_lifetime_control

  public:
    auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> override;
    bool  DelEntity( EntityId id ) override;
    auto  SetEntity( EntityId id,
      mtc::api<const IContents>         index = nullptr,
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr ) -> mtc::api<const IEntity> override;
    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override {  return this;  }
    auto  GetMaxIndex() const -> uint32_t override;
    auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override;

  protected:
    mutable std::shared_mutex ixlock;     // mutex for locking indices access
    mtc::api<IStorage>        istore;     // index access api

  };

  // ContentsIndex implementation

  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );

    return getEntity( id );
  }

  auto  ContentsIndex::GetEntity( uint32_t id ) const -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );

    return getEntity( id );
  }

  bool  ContentsIndex::DelEntity( EntityId id )
  {
    auto  shlock = mtc::make_shared_lock( ixlock );

    return delEntity( id );
  }

  auto  ContentsIndex::SetEntity( EntityId id,
    mtc::api<const IContents>         index,
    mtc::api<const mtc::IByteBuffer>  attrs ) -> mtc::api<const IEntity>
  {
    if ( indices.empty() )
      throw std::logic_error( "index flakes are not initialized" );

    for ( ; ; )
    {
      auto  shlock = mtc::make_shared_lock( ixlock );
      auto  exlock = mtc::make_unique_lock( ixlock, std::defer_lock );
      auto  pindex = indices.back().pIndex.ptr();     // the last index pointer, unchanged in one thread

    // try Set the entity to the last index in the chain
      try
      {
        auto  newdoc = pindex->SetEntity( id, index, attrs );

        return indices.back().Override( newdoc );
      }

    // on dynamic index overflow, rotate the last index by creating new one in a new flakes,
    // and continue Setting attempts
      catch ( const dynamic::index_overflow& xo )
      {
        shlock.unlock();  exlock.lock();

      // received exclusive lock, check if index is already rotated by another
      // SetEntity call; if yes, try again to SetEntity, else rotate index
        if ( indices.back().pIndex.ptr() != pindex )
          continue;

      // rotate the index by creating the commiter for last (dynamic) index
      // and create the new dynamic index
        indices.back().uUpper = indices.back().uLower
          + pindex->GetMaxIndex() - 1;

        indices.back().pIndex = Commiter::Create( indices.back().pIndex, [flakes = mtc::api<ContentsIndex>( this )]( void* index, unsigned event )
          {
            abort();
          } );

        indices.emplace_back( indices.back().uUpper + 1, dynamic::Contents()
          .SetMaxEntitiesCount( 10000 )
          .SetAllocationLimit( 2 * 1024 * 1024 )
          .SetOutStorageSink( istore->Create() ).Create() );
        indices.back().uUpper = (uint32_t)-1;
      }
    }
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
  }

  auto  ContentsIndex::GetMaxIndex() const -> uint32_t
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return getMaxIndex();
  }

  auto  ContentsIndex::GetKeyBlock( const void* key, size_t len ) const -> mtc::api<IEntities>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return getKeyBlock( key, len );
  }

  auto  ContentsIndex::GetKeyStats( const void* key, size_t len ) const -> BlockInfo
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return getKeyStats( key, len );
  }

}}}
