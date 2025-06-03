# include "../api/static-contents.hxx"
# include "static-entities.hxx"
# include <mtc/radix-tree.hpp>

namespace palmira {
namespace index {
namespace static_ {

  class ContentsIndex final: public IContentsIndex
  {
    implement_lifetime_control

  public:
    ContentsIndex( mtc::api<IStorage::ISerialized> storage );
   ~ContentsIndex() = default;

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

    auto  GetMaxIndex() const -> uint32_t override;

  protected:
    mtc::api<IStorage::ISerialized> serialized;
  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( mtc::api<IStorage::ISerialized> storage ):
    serialized( storage )
  {

  }

  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
  }

  auto  ContentsIndex::GetEntity( uint32_t id ) const -> mtc::api<const IEntity>
  {
  }

  bool  ContentsIndex::DelEntity( EntityId id )
  {
  }

  auto  ContentsIndex::SetEntity( EntityId,
    mtc::api<const IContents>,
    mtc::api<const mtc::IByteBuffer>,
    mtc::api<const mtc::IByteBuffer> ) -> mtc::api<const IEntity>
  {
    throw std::logic_error( "static_::ContentsIndex::SetEntity( ) must not be called" );
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
  }

  auto  ContentsIndex::GetMaxIndex() const -> uint32_t
  {
  }

  // contents implementation

  auto  contents::Create( mtc::api<IStorage::ISerialized> serialized ) const -> mtc::api<IContentsIndex>
    {  return new ContentsIndex( serialized );  }

}}}
