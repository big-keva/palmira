# if !defined( PALMIRA_RESTAPI_UPDATE_HPP_ )
# define PALMIRA_RESTAPI_UPDATE_HPP_
# include "modify-entity.hpp"

namespace restAPI
{

  class UpdateEntity: public ModifyEntity<UpdateEntity>
  {
    using ModifyEntity::ModifyEntity;

  public:
    void  ready() override;

  };

}
# endif // !PALMIRA_RESTAPI_UPDATE_HPP_
