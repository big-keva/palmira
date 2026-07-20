# if !defined( PALMIRA_RESTAPI_DELETE_HPP_ )
# define PALMIRA_RESTAPI_DELETE_HPP_
# include "modify-entity.hpp"

namespace restAPI
{

  class DeleteEntity: public ModifyEntity<DeleteEntity>
  {
    using ModifyEntity::ModifyEntity;

  public:
    void  ready() override;
  };

}
# endif // !PALMIRA_RESTAPI_DELETE_HPP_
