# if !defined( PALMIRA_RESTAPI_UPDATE_HPP_ )
# define PALMIRA_RESTAPI_UPDATE_HPP_
# include "doc-action.hpp"

namespace restAPI
{

  class Update: public DocAction<Update>
  {
    using DocAction::DocAction;

  public:
    void  ready() override;

  };

}

# endif // !PALMIRA_RESTAPI_UPDATE_HPP_
