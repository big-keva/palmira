# if !defined( PALMIRA_RESTAPI_DELETE_HPP_ )
# define PALMIRA_RESTAPI_DELETE_HPP_
# include "responder.hpp"

namespace restAPI
{

  class DeleteEntity: public Responder
  {
    using Responder::Responder;

  public:
    DeleteEntity( mtc::api<IService>, mtc::api<Response>, const mtc::zmap& );
    DeleteEntity( const DeleteEntity& ) = default;

    void  ready() override;

    auto  SetIndex( std::string_view ) -> DeleteEntity&;
    auto  SetDocId( std::string_view ) -> DeleteEntity&;

  protected:
    std::string   sIndex;
    std::string   sDocId;

  };

}
# endif // !PALMIRA_RESTAPI_DELETE_HPP_
