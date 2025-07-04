# if !defined( __palmira_texts_DOM_dump_hpp__ )
# define __palmira_texts_DOM_dump_hpp__
# include "DOMini.hpp"
# include <moonycode/codes.h>
# include <mtc/serialize.h>
# include <functional>
# include <stdexcept>
# include <memory>

namespace palmira {
namespace texts   {
namespace dump_as {

  auto  Tags( std::function<void(const char*, size_t)>, unsigned encode = codepages::codepage_utf8 ) -> mtc::api<IText>;
  auto  Json( std::function<void(const char*, size_t)> ) -> mtc::api<IText>;

  template <class O>
  auto  MakeOutput( O* o ) -> std::function<void(const char*, size_t)>
  {
    return [output = std::make_shared<O*>( o )]( const char* str, size_t len ) mutable
      {
        if ( (*output = ::Serialize( *output, str, len )) == nullptr )
          throw std::runtime_error( "serialization failed" );
      };
  }

}}}

# endif // !__palmira_texts_DOM_dump_hpp__
