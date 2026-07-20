# if !defined( PALMIRA_RESTAPI_INSERT_HPP_ )
# define PALMIRA_RESTAPI_INSERT_HPP_
# include "modify-entity.hpp"

namespace restAPI
{

  class InsertEntity: public ModifyEntity<InsertEntity>
  {
    using ModifyEntity::ModifyEntity;

  public:
    void  ready() override;

  };

}
# endif // !PALMIRA_RESTAPI_INSERT_HPP_
