# include "indexes/static-contents.hpp"
# include "commit-contents.hxx"
# include "override-entities.hxx"
# include "dynamic-bitmap.hxx"
# include "notify-events.hxx"
# include "patch-table.hxx"
# include <mtc/recursive_shared_mutex.hpp>
# include <condition_variable>
# include <stdexcept>
# include <thread>
#include <mtc/json.h>

namespace palmira {
namespace index   {
namespace commit  {

  class ContentsIndex final: public IContentsIndex
  {
    using ISerialized = IStorage::ISerialized;

    implement_lifetime_control

  public:
    ContentsIndex( mtc::api<IContentsIndex>, Notify::Func );
   ~ContentsIndex();

  public:
    auto  StartCommit() -> mtc::api<IContentsIndex>;

  public:     // overridables
    auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> override;

    bool  DelEntity( EntityId ) override;
    auto  SetEntity( EntityId, mtc::api<const IContents>,
      const Span& ) -> mtc::api<const IEntity> override;
    auto  SetExtras( EntityId,
      const Span& ) -> mtc::api<const IEntity> override;

    auto  GetMaxIndex() const -> uint32_t override;
    auto  GetKeyBlock( const Span& ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const Span& ) const -> BlockInfo override;

    auto  GetEntityIterator( EntityId ) -> mtc::api<IEntityIterator> override
      {  throw std::runtime_error( "not implemented" );  }
    auto  GetEntityIterator( uint32_t ) -> mtc::api<IEntityIterator> override
      {  throw std::runtime_error( "not implemented" );  }

    auto  GetRecordIterator( const Span& ) -> mtc::api<IRecordIterator> override
      {  throw std::runtime_error( "not implemented" );  }

    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override;
    void  Stash( EntityId ) override  {}

  protected:
    void  CommitThreadFunc();

  protected:
    mutable std::shared_mutex     swLock;   // switch mutex
    mtc::api<IContentsIndex>      source;
    mtc::api<IContentsIndex>      output;
    mtc::api<ISerialized>         serial;
    Notify::Func                  notify;

    std::mutex                    s_lock;   // notification
    std::condition_variable       s_wait;

    Bitmap<>                      banset;
    mutable PatchTable<>          hpatch;

    std::thread                   commit;
    std::exception_ptr            except;

  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( mtc::api<IContentsIndex> src, Notify::Func fnu ):
    source( src ),
    notify( fnu ),
    banset( source->GetMaxIndex() + 1 ),
    hpatch( std::max( 1000U, (source->GetMaxIndex() + 1) / 3 ) ) {}

  ContentsIndex::~ContentsIndex()
  {
    if ( commit.joinable() )
      commit.join();
  }

  auto  ContentsIndex::StartCommit() -> mtc::api<IContentsIndex>
  {
    commit = std::thread( &ContentsIndex::CommitThreadFunc, this );
    return this;
  }

  void  ContentsIndex::CommitThreadFunc()
  {
    pthread_setname_np( pthread_self(), "commit::Flush()" );

  // first commit index to the storage
  // then try open the new static index from the storage
    try
    {
      auto  target = source->Commit();    // serialize the dynamic index itself
      auto  exlock = mtc::make_unique_lock( swLock );

    // serialize accumulated changes, dispose old index and open new static
      hpatch.Commit( serial = target );
        source = nullptr;
      output = static_::Contents().Create( serial = target );

    // notify commit finished
      s_wait.notify_all();

    // notify serialize finished
      if ( notify != nullptr )
        notify( this, Notify::Event::OK );
    }
    catch ( ... )
    {
      except = std::current_exception();

      if ( notify != nullptr )
        notify( this, Notify::Event::Failed );
    }
  }

  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( swLock );

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( output == nullptr )
    {
      auto  entity = source->GetEntity( id );
      auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

      if ( entity == nullptr || (ppatch = hpatch.Search( { id.data(), id.size() } )) == nullptr )
        return entity;

      if ( ppatch->GetLen() == size_t(-1) )
        return nullptr;

      return Override::Entity( entity ).Extra( ppatch );
    }
    return output->GetEntity( id );
  }

  auto  ContentsIndex::GetEntity( uint32_t ix ) const -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( swLock );

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( output == nullptr )
    {
      auto  entity = source->GetEntity( ix );
      auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

      if ( entity == nullptr || (ppatch = hpatch.Search( ix )) == nullptr )
        return entity;

      if ( ppatch->GetLen() == size_t(-1) )
        return nullptr;

      return Override::Entity( entity ).Extra( ppatch );
    }
    return output->GetEntity( ix );
  }

  bool  ContentsIndex::DelEntity( EntityId id )
  {
    auto  shlock = mtc::make_shared_lock( swLock );

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( output == nullptr )
    {
      auto  entity = source->GetEntity( id );
      auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

      if ( entity == nullptr )
        return false;

      if ( (ppatch = hpatch.Search( { id.data(), id.size() } )) == nullptr || ppatch->GetLen() != size_t(-1) )
      {
        hpatch.Delete( { id.data(), id.size() }, entity->GetIndex() );
        banset.Set( entity->GetIndex() );
        return true;
      }
      return false;
    }
    return output->DelEntity( id );
  }

  auto  ContentsIndex::SetEntity( EntityId, mtc::api<const IContents>, const Span& ) -> mtc::api<const IEntity>
  {
    throw std::logic_error( "commit::SetEntity(...) must not be called" );
  }

  auto  ContentsIndex::SetExtras( EntityId id, const Span& xtra ) -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( swLock );

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( output == nullptr )
    {
      auto  entity = source->GetEntity( id );
      auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

      if ( entity == nullptr )
        return nullptr;

      if ( (ppatch = hpatch.Search( { id.data(), id.size() } )) != nullptr && ppatch->GetLen() == size_t(-1) )
        return nullptr;

      return hpatch.Update( { id.data(), id.size() }, entity->GetIndex(), xtra )->GetLen() != size_t(-1) ?
        GetEntity( id ) : nullptr;
    }
    return output->SetExtras( id, xtra );
  }

  auto  ContentsIndex::GetMaxIndex() const -> uint32_t
  {
    auto  shlock = mtc::make_shared_lock( swLock );

    if ( except != nullptr )
      std::rethrow_exception( except );

    return output != nullptr ? output->GetMaxIndex() : source->GetMaxIndex();
  }

  auto  ContentsIndex::GetKeyBlock( const Span& key ) const -> mtc::api<IEntities>
  {
    auto  shlock = mtc::make_shared_lock( swLock );
    auto  pblock = mtc::api<IEntities>();

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( output != nullptr )
      return output->GetKeyBlock( key );

    if ( (pblock = source->GetKeyBlock( key )) == nullptr )
      return nullptr;

    return new Override::Entities( pblock, banset, this );
  }

  auto  ContentsIndex::GetKeyStats( const Span& key ) const -> BlockInfo
  {
    auto  shlock = mtc::make_shared_lock( swLock );

    if ( except != nullptr )
      std::rethrow_exception( except );

    return (output != nullptr ? output : source)->GetKeyStats( key );
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
    std::unique_lock<std::mutex>  exlock( s_lock );
      s_wait.wait( exlock, [&](){  return serial != nullptr;  } );
    return serial;
  }

  auto  ContentsIndex::Reduce() -> mtc::api<IContentsIndex>
  {
  // wait until the commit completes
    if ( commit.joinable() )
      commit.join();

    if ( except != nullptr )
      std::rethrow_exception( except );

    return output;
  }

  // Commit implementation

  auto  Contents::Create( mtc::api<IContentsIndex> src, Notify::Func nfn ) -> mtc::api<IContentsIndex>
  {
    return (new ContentsIndex( src, nfn ))->StartCommit();
  }

}}}
