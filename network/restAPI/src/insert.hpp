# if !defined( PALMIRA_RESTAPI_INSERT_HPP_ )
# define PALMIRA_RESTAPI_INSERT_HPP_
# include "doc-action.hpp"

namespace restAPI
{

  class Insert: public DocAction<Insert>
  {
    using DocAction::DocAction;

  public:
    void  ready() override;

  };

}

# endif // !PALMIRA_RESTAPI_INSERT_HPP_
