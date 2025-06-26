# if !defined( __palmira_src_index_commit_contents_hxx__ )
# define __palmira_src_index_commit_contents_hxx__
# include "contents-index.hpp"
# include "notify-events.hxx"

namespace palmira {
namespace index   {
namespace commit {

  struct Contents
  {
    auto  Create( mtc::api<IContentsIndex>, Notify::Func ) -> mtc::api<IContentsIndex>;
  };

}}}

# endif // !__palmira_src_index_commit_contents_hxx__
