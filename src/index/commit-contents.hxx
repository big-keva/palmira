# include "../../api/contents-index.hxx"
# include <functional>

namespace palmira {
namespace index   {

  struct Notify final
  {
    enum: unsigned
    {
      OK = 1,
      Cancel = 2,
      Failed = 3
    };

    using Update = std::function<void( void*, unsigned )>;
  };

  struct Commiter
  {
    static  auto  Create( mtc::api<IContentsIndex>, Notify::Update ) -> mtc::api<IContentsIndex>;
  };

}}