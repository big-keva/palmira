# if !defined( __palmira_readers_read_zip_hpp__ )
# define __palmira_readers_read_zip_hpp__
# include <mtc/span.hpp>
# include <functional>
# include <vector>
# include <string>

namespace palmira {
namespace minizip {

  using ReadTextFn = std::function<void(
    const mtc::span<const char>&, const std::vector<std::string>& )>;

  void  Read( ReadTextFn, const mtc::span<const char>&, const std::vector<std::string>& = {} );

}}

# endif   // !__palmira_readers_read_zip_hpp__
