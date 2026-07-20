# if !defined( PALMIRA_RESTAPI_UPDATE_HPP_ )
# define PALMIRA_RESTAPI_UPDATE_HPP_
# include "responder.hpp"

namespace restAPI
{

  class UpdateEntity: public Responder
  {
    using Responder::Responder;

  public:
    UpdateEntity( mtc::api<IService>, mtc::api<Response>, const mtc::zmap& );
    UpdateEntity( const UpdateEntity& ) = default;

    void  ready() override;

    auto  SetIndex( std::string_view ) -> UpdateEntity&;
    auto  SetDocId( std::string_view ) -> UpdateEntity&;

  protected:
    std::string   sIndex;
    std::string   sDocId;

  };

}
# endif // !PALMIRA_RESTAPI_UPDATE_HPP_
