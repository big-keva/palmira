# if !defined( __palmira_storage_posix_fs_hpp__ )
# define __palmira_storage_posix_fs_hpp__
# include "contents-index.hpp"
# include <string_view>

namespace palmira {
namespace storage {
namespace posixFS {

  enum Unit: unsigned
  {
    entities      = 0x0001,
    attributes    = 0x0002,
    contents      = 0x0004,
    blocks        = 0x0008,
    images        = 0x0010,
    status        = 0x0020
  };

  enum Mode: unsigned
  {
    preloaded     = 0,
    memory_mapped = 1,
    file_based    = 2
  };

  struct Policy
  {
    const Unit        unit;
    const Mode        mode;
    const std::string path;

  public:
    auto  GetFilePath( Unit, const char* stamp = nullptr ) const -> std::string;

  };

  class StoragePolicies
  {
    struct Impl;

    Impl*  impl = nullptr;

  public:
    StoragePolicies() = default;
    StoragePolicies( std::initializer_list<Policy> );
    StoragePolicies( const Policy*, size_t );
    template <class It>
    StoragePolicies( It, It );
    StoragePolicies( StoragePolicies&& );
    StoragePolicies( const StoragePolicies& );
   ~StoragePolicies();

    auto  operator=( const StoragePolicies& ) -> StoragePolicies&;

    auto  GetInstance( const char* ) const -> StoragePolicies;
    auto  GetInstance( const std::string& ) const -> StoragePolicies;

    bool  IsInstance() const;

  public:
    static  auto  Open( const std::string& ) -> StoragePolicies;
    static  auto  OpenInstance( const std::string& ) -> StoragePolicies;

  public:
    auto  AddPolicy( const Policy& ) -> StoragePolicies&;
    auto  AddPolicy( Unit, Mode, std::string_view ) -> StoragePolicies&;
    auto  GetPolicy( Unit ) const -> const Policy*;
    static
    auto  GetSuffix( Unit) -> const char*;

  };

  auto  CreateSink( const StoragePolicies& ) -> mtc::api<IStorage::IIndexStore>;
  auto  OpenSerial( const StoragePolicies& ) -> mtc::api<IStorage::ISerialized>;

  auto  Open( const StoragePolicies& ) -> mtc::api<IStorage>;

  // StoragePolicies template implementation

  template <class It>
  StoragePolicies::StoragePolicies( It start, It end )
  {
    for ( ; start != end; ++start )
      AddPolicy( *start );
  }

}}}

# endif   // !__palmira_storage_posix_fs_hpp__
