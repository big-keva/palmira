# if !defined( __palmira_context_make_image_hpp__ )
# define __palmira_context_make_image_hpp__
# include "contents-index.hpp"

namespace palmira {
namespace context {

  struct Lite final
  {
    static  auto  Create() -> mtc::api<IImage>;
  };

  struct BM25 final
  {
    static  auto  Create() -> mtc::api<IImage>;
  };

  struct Rich final
  {
    static  auto  Create() -> mtc::api<IImage>;
  };

}}

# endif // !__palmira_context_make_image_hpp__
