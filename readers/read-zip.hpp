# if !defined( __palmira_readers_read_zip_hpp__ )
# define __palmira_readers_read_zip_hpp__
# include "DelphiX/slices.hpp"
# include <functional>
# include <vector>
# include <string>

namespace palmira {
namespace minizip {

  using ReadTextFn = std::function<void(
    const DelphiX::Slice<const char>&, const std::vector<std::string>& )>;

  void  Read( ReadTextFn, const DelphiX::Slice<const char>&, const std::vector<std::string>& = {} );

}}

# endif   // !__palmira_readers_read_zip_hpp__
