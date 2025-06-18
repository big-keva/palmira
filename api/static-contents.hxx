# if !defined( __palmira_api_index_static_contents_hxx__ )
# define __palmira_api_index_static_contents_hxx__
# include "contents-index.hxx"

namespace palmira {
namespace index {
namespace static_ {

  struct Contents
  {
    auto  Create( mtc::api<IStorage::ISerialized> ) -> mtc::api<IContentsIndex>;
  };

}}}

# endif   // !__palmira_api_index_static_contents_hxx__
