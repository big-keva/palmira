# if !defined( __palmira_readers_read_xml_hpp__ )
# define __palmira_readers_read_xml_hpp__
#include <DelphiX/slices.hpp>

# include "DelphiX/text-api.hpp"

namespace palmira {
namespace tinyxml {

  class Error: public std::runtime_error
    {  using std::runtime_error::runtime_error;  };

  enum class TagMode: unsigned
  {
    undefined = 0,
    ignore = 1,     // ignore tag
    remove = 2      // suppress contents
  };

  using TagModes = std::initializer_list<std::pair<const std::string, TagMode>>;

  void  Read( DelphiX::textAPI::IText*, const TagModes&, const DelphiX::Slice<const char>& );
  void  Read( DelphiX::textAPI::IText*, const TagModes&, FILE* );

}}

# endif   // !__palmira_readers_read_xml_hpp__
