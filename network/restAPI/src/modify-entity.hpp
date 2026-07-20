# if !defined( PALMIRA_RESTAPI_MODIFY_HPP_ )
# define PALMIRA_RESTAPI_MODIFY_HPP_
# include "responder.hpp"

namespace restAPI
{

  template <class SelfType>
  class ModifyEntity: public Responder
  {
    using Responder::Responder;

  public:
    ModifyEntity( mtc::api<IService>, mtc::api<Response>, const mtc::zmap& );
    ModifyEntity( const ModifyEntity& ) = default;

    void  ready() override;

    auto  SetSpace( std::string_view ) -> SelfType&;
    auto  SetDocId( std::string_view ) -> SelfType&;

  protected:
    std::string space;
    std::string docId;

  };

  template <class SelfType>
  auto  ModifyEntity<SelfType>::SetSpace( std::string_view ix ) -> SelfType&
  {
    return space = ix, *(SelfType*)this;
  }

  template <class SelfType>
  auto  ModifyEntity<SelfType>::SetDocId( std::string_view id ) -> SelfType&
  {
    return docId = id, *(SelfType*)this;
  }

}

# endif // !PALMIRA_RESTAPI_MODIFY_HPP_
