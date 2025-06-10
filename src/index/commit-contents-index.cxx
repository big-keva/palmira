# include "commit-contents-index.hxx"
# include "dynamic-bitmap.hxx"
# include "patch-table.hxx"
# include "../../api/static-contents.hxx"
# include <stdexcept>
# include <thread>
# include <mtc/radix-tree.hpp>

namespace palmira {
namespace index   {

  class Commiter final: public IContentsIndex
  {
    implement_lifetime_control

  public:
    Commiter( mtc::api<IContentsIndex>, NotifyUpdate notifyUpdate );
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
    mtc::api<IContentsIndex>        loaded;
    mtc::api<IContentsIndex>        stored;
    NotifyUpdate                    notify;

    Bitmap<>                        banset;

    std::atomic<IContentsIndex*>    target;
    std::thread                     commit;
    std::exception_ptr              except;

  };

  // Commiter implementation

  Commiter::Commiter( mtc::api<IContentsIndex> src, NotifyUpdate fnu ):
    stored( src ),
    notify( fnu ),
    banset( stored->GetMaxIndex() + 1 ),
    target( src.ptr() )  {}

  auto  Commiter::BeginCommit() -> mtc::api<IContentsIndex>
  {
    commit = std::thread( &Commiter::FlushFunc, this );
    return this;
  }

  void  Commiter::FlushFunc()
  {
    auto  serial = mtc::api<IStorage::ISerialized>();

  // first commit index to the storage
  // then try open the new static index from the storage
    try
    {
      serial = stored->Commit();
      loaded = Static::Create( serial );
      target.store( loaded.ptr() );
      notify( this );
    }
    catch ( ... )
    {
      return (void)(except = std::current_exception());
    }
  }

  auto  Commiter::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
  }

  auto  Commiter::GetEntity( uint32_t ix ) const -> mtc::api<const IEntity>
  {
  }

  bool  Commiter::DelEntity( EntityId id )
  {
  }

  auto  Commiter::SetEntity( EntityId id, mtc::api<const IContents>,
    mtc::api<const mtc::IByteBuffer> ) -> mtc::api<const IEntity>
  {
    throw std::logic_error( "Commiter::SetEntity(...) must not be called" );
  }

  auto  Commiter::Commit() -> mtc::api<IStorage::ISerialized>
  {

  }

  auto  Commiter::Reduce() -> mtc::api<IContentsIndex>
  {

  }

  auto  Commiter::GetMaxIndex() const -> uint32_t
  {
    return target.load()->GetMaxIndex();
  }

  auto  Commiter::GetKeyBlock( const void* key, size_t length ) const -> mtc::api<IEntities>
  {
    return target.load()->GetKeyBlock( key, length );
  }

  auto  Commiter::GetKeyStats( const void* key, size_t length ) const -> BlockInfo
  {
    return target.load()->GetKeyStats( key, length );
  }

  // Commit implementation

  auto  Commit::Create( mtc::api<IContentsIndex> src, NotifyUpdate nfn ) -> mtc::api<IContentsIndex>
  {
    return (new Commiter( src, nfn ))->BeginCommit();
  }

}}
