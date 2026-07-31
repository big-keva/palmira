# if !defined( PALMIRA_RESTAPI_DELETE_HPP_ )
# define PALMIRA_RESTAPI_DELELE_HPP_
# include "doc-action.hpp"

namespace restAPI
{

  class Delete: public DocAction<Delete>
  {
    using DocAction::DocAction;

  public:
    void  ready() override;
  };

}
# endif // !PALMIRA_RESTAPI_DELETE_HPP_
