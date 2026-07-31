# if !defined( PALMIRA_RESTAPI_SEARCH_HPP_ )
# define PALMIRA_RESTAPI_SEARCH_HPP_
# include "doc-action.hpp"

namespace restAPI
{

  class Search: public DocAction<Search>
  {
    using DocAction::DocAction;

  public:
    void  ready() override;

  };

}

# endif // !PALMIRA_RESTAPI_SEARCH_HPP_
