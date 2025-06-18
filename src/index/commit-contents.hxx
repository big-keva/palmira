# include "../../api/contents-index.hxx"
# include "notify-events.hxx"
# include <functional>

namespace palmira {
namespace index   {
namespace commit {

  struct Contents
  {
    auto  Create( mtc::api<IContentsIndex>, Notify::Func ) -> mtc::api<IContentsIndex>;
  };

}}}