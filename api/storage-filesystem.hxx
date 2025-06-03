# if !defined( __palmira_api_storage_filesystem_hxx__ )
# define __palmira_api_storage_filesystem_hxx__
# include "../api/contents-index.hxx"
# include <string_view>

namespace palmira {
namespace storage {
namespace filesys {

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
    StoragePolicies( const char* );
    StoragePolicies( const std::string& );
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

  public:
    auto  AddPolicy( const Policy& ) -> StoragePolicies&;
    auto  AddPolicy( Unit, Mode, std::string_view ) -> StoragePolicies&;
    auto  GetPolicy( Unit ) const -> const Policy*;
    static
    auto  GetSuffix( Unit) -> const char*;

  };

  auto  CreateSink( const StoragePolicies& ) -> mtc::api<IStorage::IIndexStore>;
  auto  OpenSerial( const StoragePolicies& ) -> mtc::api<IStorage::ISerialized>;

  // StoragePolicies template implementation

  template <class It>
  StoragePolicies::StoragePolicies( It start, It end )
  {
    for ( ; start != end; ++start )
      AddPolicy( *start );
  }

}}}

# endif   // !__palmira_api_storage_filesystem_hxx__
