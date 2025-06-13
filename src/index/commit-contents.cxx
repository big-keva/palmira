# include "commit-contents.hxx"
# include "override-entities.hxx"
# include "dynamic-bitmap.hxx"
# include "patch-table.hxx"
# include "../../api/static-contents.hxx"
# include <stdexcept>
# include <thread>
# include <mtc/radix-tree.hpp>

namespace palmira {
namespace index   {

  class CommitIndex final: public IContentsIndex
  {
    implement_lifetime_control

  public:
    CommitIndex( mtc::api<IContentsIndex>, Notify::Update notifyUpdate );
   ~CommitIndex();
    auto  BeginCommit() -> mtc::api<IContentsIndex>;

  protected:
    void  FlushFunc();

  public:     // overridables
    auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> override;
    bool  DelEntity( EntityId id ) override;
    auto  SetEntity( EntityId id,
      mtc::api<const IContents>         index = nullptr,
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr ) -> mtc::api<const IEntity> override;
    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override;
    auto  GetMaxIndex() const -> uint32_t override;
    auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override;

  protected:
    mtc::api<IContentsIndex>        source;
    mtc::api<IContentsIndex>        output;
    Notify::Update                  notify;

    Bitmap<>                        banset;
    mutable PatchTable<>            hpatch;

    std::atomic<IContentsIndex*>    target;
    std::thread                     commit;
    std::exception_ptr              except;

  };

  // CommitIndex implementation

  CommitIndex::CommitIndex( mtc::api<IContentsIndex> src, Notify::Update fnu ):
    source( src ),
    notify( fnu ),
    banset( source->GetMaxIndex() + 1 ),
    hpatch( std::max( 1000U, (source->GetMaxIndex() + 1) / 3 ) ),
    target( src.ptr() )  {}

  CommitIndex::~CommitIndex()
  {
    if ( commit.joinable() )
      commit.join();
  }

  auto  CommitIndex::BeginCommit() -> mtc::api<IContentsIndex>
  {
    commit = std::thread( &CommitIndex::FlushFunc, this );
    return this;
  }

  void  CommitIndex::FlushFunc()
  {
    auto  serial = mtc::api<IStorage::ISerialized>();

  // first commit index to the storage
  // then try open the new static index from the storage
    try
    {
      serial = source->Commit();
      output = Static::Create( serial );
      target.store( output.ptr() );

      if ( notify != nullptr )
        notify( this, Notify::OK );
    }
    catch ( ... )
    {
      except = std::current_exception();

      if ( notify != nullptr )
        notify( this, Notify::Failed );
    }
  }

  auto  CommitIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    auto  pindex = target.load();
    auto  entity = pindex != nullptr ? pindex->GetEntity( id ) : nullptr;
    auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( entity == nullptr )
      return nullptr;

    if ( (ppatch = hpatch.Search( { id.data(), id.size() } )) == nullptr )
      return entity;

    if ( ppatch->GetLen() == size_t(-1) )
      return nullptr;

    return Override( entity ).Attributes( ppatch );
  }

  auto  CommitIndex::GetEntity( uint32_t ix ) const -> mtc::api<const IEntity>
  {
    auto  pindex = target.load();
    auto  entity = pindex != nullptr ? pindex->GetEntity( ix ) : nullptr;
    auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( entity == nullptr )
      return nullptr;

    if ( (ppatch = hpatch.Search( ix )) == nullptr )
      return entity;

    if ( ppatch->GetLen() == size_t(-1) )
      return nullptr;

    return Override( entity ).Attributes( ppatch );
  }

  bool  CommitIndex::DelEntity( EntityId id )
  {
    auto  pindex = target.load();
    auto  entity = pindex != nullptr ? pindex->GetEntity( id ) : nullptr;
    auto  ppatch = mtc::api<const mtc::IByteBuffer>{};

    if ( except != nullptr )
      std::rethrow_exception( except );

    if ( entity == nullptr )
      return false;

    if ( (ppatch = hpatch.Search( { id.data(), id.size() } )) == nullptr || ppatch->GetLen() != size_t(-1) )
    {
      hpatch.Delete( { id.data(), id.size() } );
      hpatch.Delete( entity->GetIndex() );
      banset.Set( entity->GetIndex() );
      return true;
    }
    return false;
  }

  auto  CommitIndex::SetEntity( EntityId, mtc::api<const IContents>, mtc::api<const mtc::IByteBuffer> )
    -> mtc::api<const IEntity>
  {
    throw std::logic_error( "CommitIndex::SetEntity(...) must not be called" );
  }

  auto  CommitIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {

  }

  auto  CommitIndex::Reduce() -> mtc::api<IContentsIndex>
  {
  // wait until the commit completes
    if ( commit.joinable() )
      commit.join();
    return target.load();
  }

  auto  CommitIndex::GetMaxIndex() const -> uint32_t
  {
    if ( except != nullptr )
      std::rethrow_exception( except );

    return target.load()->GetMaxIndex();
  }

  auto  CommitIndex::GetKeyBlock( const void* key, size_t length ) const -> mtc::api<IEntities>
  {
    if ( except != nullptr )
      std::rethrow_exception( except );

    return target.load()->GetKeyBlock( key, length );
  }

  auto  CommitIndex::GetKeyStats( const void* key, size_t length ) const -> BlockInfo
  {
    if ( except != nullptr )
      std::rethrow_exception( except );

    return target.load()->GetKeyStats( key, length );
  }

  // Commit implementation

  auto  Commiter::Create( mtc::api<IContentsIndex> src, Notify::Update nfn ) -> mtc::api<IContentsIndex>
  {
    return (new CommitIndex( src, nfn ))->BeginCommit();
  }

}}
