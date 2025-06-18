# if !defined( __palmira_api_index_dynamic_contents_hxx__ )
# define __palmira_api_index_dynamic_contents_hxx__
# include "contents-index.hxx"

namespace palmira {
namespace index {
namespace dynamic {

  struct Settings
  {
    uint32_t  maxEntities = 2000;                 /* */
    uint32_t  maxAllocate = 256 * 1024 * 1024;    /* 256 meg */

  public:
    auto  SetMaxEntities( uint32_t value ) -> Settings& {  maxEntities = value; return *this;  }
    auto  SetMaxAllocate( uint32_t value ) -> Settings& {  maxAllocate = value; return *this;  }
  };

  class Contents
  {
    using Storage = mtc::api<IStorage::IIndexStore>;

    Settings  openOptions;
    Storage   storageSink;

  public:
    auto  Set( const Settings& ) -> Contents&;
    auto  Set( mtc::api<IStorage::IIndexStore> ) -> Contents&;

  public:
    auto  Create() const -> mtc::api<IContentsIndex>;
  };

}}}

# endif   // !__palmira_api_index_dynamic_contents_hxx__
