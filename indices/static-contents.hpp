# if !defined( __palmira_indices_static_contents_hpp__ )
# define __palmira_indices_static_contents_hpp__
# include "contents-index.hpp"

namespace palmira {
namespace index {
namespace static_ {

  struct Contents
  {
    auto  Create( mtc::api<IStorage::ISerialized> ) -> mtc::api<IContentsIndex>;
  };

}}}

# endif   // !__palmira_indices_static_contents_hpp__
