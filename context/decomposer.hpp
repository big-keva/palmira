# if !defined( __palmira_decomposer_hpp__ )
# define __palmira_decomposer_hpp__
# include "lang-api.hpp"
# include <functional>

namespace palmira {
namespace query {

  using Decomposer = std::function<void( IImage*,
    const Slice<TextToken>&,
    const Slice<MarkupTag>& )>;

  class DefaultDecomposer
  {
  public:
    void  operator()( IImage*, const Slice<TextToken>&, const Slice<MarkupTag>& );
  };

  auto  LoadDecomposer( const char*, const char* ) -> Decomposer;

}};

# endif // !__palmira_decomposer_hpp__
