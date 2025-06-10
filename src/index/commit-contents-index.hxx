# include "../../api/contents-index.hxx"
# include <functional>

namespace palmira {
namespace index   {

  using NotifyUpdate = std::function<void( void* )>;

  struct Commit
  {
    static  auto  Create( mtc::api<IContentsIndex>, NotifyUpdate ) -> mtc::api<IContentsIndex>;
  };

}}