# if !defined( PALMIRA_RESTAPI_GET_HPP_ )
# define PALMIRA_RESTAPI_GET_HPP_
# include "doc-action.hpp"

namespace restAPI
{

  class GetEntity: public DocAction<GetEntity>
  {
    using DocAction::DocAction;

  public:
    void  ready() override;
  };

}
# endif // !PALMIRA_RESTAPI_GET_HPP_
