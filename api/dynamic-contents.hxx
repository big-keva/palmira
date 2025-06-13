# if !defined( __palmira_api_index_dynamic_contents_hxx__ )
# define __palmira_api_index_dynamic_contents_hxx__
# include "contents-index.hxx"

namespace palmira {
namespace index {
namespace dynamic {

  class Contents
  {
    using Storage = mtc::api<IStorage::IIndexStore>;

    uint32_t  maxEntities = 2000;                 /* */
    uint32_t  maxAllocate = 256 * 1024 * 1024;    /* 256 meg */
    Storage   storageSink;

  public:
    auto  SetMaxEntitiesCount( uint32_t maxEntitiesCount ) -> Contents&;
    auto  SetAllocationLimit( uint32_t maxAllocateBytes ) -> Contents&;
    auto  SetOutStorageSink( mtc::api<IStorage::IIndexStore> ) -> Contents&;

  public:
    auto  Create() const -> mtc::api<IContentsIndex>;
  };

}}}

# endif   // !__palmira_api_index_dynamic_contents_hxx__
