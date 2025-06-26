# if !defined( __palmira_src_index_fusion_h__ )
# define __palmira_src_index_fusion_h__
# include "contents-index.hpp"
# include <functional>

namespace palmira {
namespace index   {
namespace fusion {

  class ContentsMerger
  {
    std::function<bool()>   canContinue = [](){  return true;  };

  public:
    ContentsMerger() = default;

    auto  Add( mtc::api<IContentsIndex> ) -> ContentsMerger&;
    auto  Set( std::function<bool()> ) -> ContentsMerger&;
    auto  Set( mtc::api<IStorage::IIndexStore> ) -> ContentsMerger&;
    auto  Set( const mtc::api<IContentsIndex>*, size_t ) -> ContentsMerger&;
    auto  Set( const std::vector<mtc::api<IContentsIndex>>& ) -> ContentsMerger&;
    auto  Set( const std::initializer_list<const mtc::api<IContentsIndex>>& ) -> ContentsMerger&;

    auto  operator()() -> mtc::api<IStorage::ISerialized>;

  protected:
    void  MergeEntities();
    void  MergeContents();

  protected:
    mtc::api<IStorage::IIndexStore>       storage;
    std::vector<mtc::api<IContentsIndex>> indices;
    std::vector<std::vector<uint32_t>>    remapId;

  };

}}}

# endif // !__palmira_src_index_fusion_h__
