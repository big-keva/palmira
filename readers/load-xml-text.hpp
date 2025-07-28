# if !defined( __palmira_readers_load_xml_text_hpp__ )
# define __palmira_readers_load_xml_text_hpp__
# include "DelphiX/text-api.hpp"

namespace palmira {
namespace tinyxml {

  enum class TagMode: unsigned
  {
    undefined = 0,
    ignore = 1,     // ignore tag
    remove = 2      // suppress contents
  };

  using TagModes = std::initializer_list<std::pair<const std::string, TagMode>>;

  void  LoadObject( DelphiX::textAPI::IText*, const TagModes&, const char*, size_t );
  void  LoadObject( DelphiX::textAPI::IText*, const TagModes&, FILE* );

}}

# endif   // !__palmira_readers_load_xml_text_hpp__
