# include "../../api/storage-filesystem.hxx"
# include <mtc/fileStream.h>
# include <mtc/wcsstr.h>
# include <stdexcept>
# include <thread>
# include <unistd.h>

namespace palmira {
namespace storage {
namespace filesys {

  class Serial final: public IStorage::ISerialized
  {
    implement_lifetime_control

  public:
    Serial( const StoragePolicies& pol ):
      policies( pol ) {}

  public:
    auto  Entities() -> mtc::api<const mtc::IByteBuffer> override;
    auto  Contents() -> mtc::api<const mtc::IByteBuffer> override;
    auto  Blocks() -> mtc::api<const mtc::IFlatStream> override;
    auto  Images() -> mtc::api<IStorage::IImageStore> override;

    auto  Commit() -> mtc::api<ISerialized> override;
    void  Remove() override;

    auto  NewPatch() -> mtc::api<IPatch> override;

  protected:
    const StoragePolicies                 policies;

    mtc::api<const mtc::IByteBuffer>      entities;
    mtc::api<const mtc::IByteBuffer>      contents;
    mtc::api<const mtc::IFlatStream>      blocks;
    mtc::api<const IStorage::IImageStore> images;

  };

  // Serial implementation

  auto  Serial::Entities() -> mtc::api<const mtc::IByteBuffer>
  {
    return nullptr;
  }

  auto  Serial::Contents() -> mtc::api<const mtc::IByteBuffer>
  {
    return nullptr;
  }

  auto  Serial::Blocks() -> mtc::api<const mtc::IFlatStream>
  {
    return nullptr;
  }

  auto  Serial::Images() -> mtc::api<IStorage::IImageStore>
  {
    return nullptr;
  }

  auto  Serial::Commit() -> mtc::api<ISerialized>
  {
    return this;
  }

  void  Serial::Remove()
  {
    entities = nullptr;  blocks = nullptr;
    contents = nullptr;  images = nullptr;

    for ( auto unit: { Unit::entities, Unit::blocks, Unit::contents, Unit::images, Unit::status } )
    {
      auto  policy = policies.GetPolicy( unit );

      if ( policy != nullptr )
        remove( policy->GetFilePath( unit ).c_str() );
    }
  }

  auto  Serial::NewPatch() -> mtc::api<IPatch>
  {
    return nullptr;
  }

  auto  OpenSerial( const StoragePolicies& policies ) -> mtc::api<IStorage::ISerialized>
  {
    return new Serial( policies );
  }

}}}
