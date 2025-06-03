# if !defined( __palmira_api_index_dynamic_contents_hxx__ )
# define __palmira_api_index_dynamic_contents_hxx__
# include "contents-index.hxx"

namespace palmira {
namespace index {
namespace dynamic {

  class contents
  {
    using Storage = mtc::api<IStorage::IIndexStore>;

    uint32_t  maxEntities = 2000;                 /* */
    uint32_t  maxAllocate = 256 * 1024 * 1024;    /* 256 meg */
    Storage   storageSink;

  public:
    auto  SetMaxEntitiesCount( uint32_t maxEntitiesCount ) -> contents&;
    auto  SetAllocationLimit( uint32_t maxAllocateBytes ) -> contents&;
    auto  SetOutStorageSink( mtc::api<IStorage::IIndexStore> ) -> contents&;

  public:
    auto  Create() const -> mtc::api<IContentsIndex>;
  };

}}}

# endif   // !__palmira_api_index_dynamic_contents_hxx__
