# include "override-entities.hxx"

# include <functional>

namespace palmira {
namespace index   {

  class override_index final: public IEntity
  {
    mtc::api<const IEntity> entity;
    uint32_t                oindex;

    implement_lifetime_control

  public:
    override_index( mtc::api<const IEntity> en, uint32_t ix ):
      entity( en ),
      oindex( ix ) {}

  protected:
    auto  GetId() const -> EntityId override {  return entity->GetId();  }
    auto  GetIndex() const -> uint32_t override {  return oindex;  }
    auto  GetExtra() const -> mtc::api<const mtc::IByteBuffer> override {  return entity->GetExtra();  }
    auto  GetVersion() const -> uint64_t override {  return entity->GetVersion();  }

  };

  class override_attributes final: public IEntity
  {
    mtc::api<const IEntity>           entity;
    mtc::api<const mtc::IByteBuffer>  aprops;

    implement_lifetime_control

  public:
    override_attributes( mtc::api<const IEntity> en, const mtc::api<const mtc::IByteBuffer>& bb ):
      entity( en ),
      aprops( bb ) {}

  protected:
    auto  GetId() const -> EntityId override {  return entity->GetId();  }
    auto  GetIndex() const -> uint32_t override {  return entity->GetIndex();  }
    auto  GetExtra() const -> mtc::api<const mtc::IByteBuffer> override {  return aprops;  }
    auto  GetVersion() const -> uint64_t override {  return entity->GetVersion();  }

  };

  // Override implementation

  Override::Override( mtc::api<const IEntity> p ):
    entity( p ) {}

  auto  Override::Index( uint32_t ix ) -> mtc::api<const IEntity>
  {
    return new override_index( entity, ix );
  }

  auto  Override::Extras( const mtc::api<const mtc::IByteBuffer>& bb ) -> mtc::api<const IEntity>
  {
    return new override_attributes( entity, bb );
  }

}}
