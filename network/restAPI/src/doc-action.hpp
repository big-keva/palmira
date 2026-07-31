# if !defined( PALMIRA_RESTAPI_ACTION_HPP_ )
# define PALMIRA_RESTAPI_ACTION_HPP_
# include "responder.hpp"

namespace restAPI
{

  template <class SelfType>
  class DocAction: public Responder
  {
    using Responder::Responder;

  public:
    DocAction( mtc::api<IService>, mtc::api<Response>, std::string_view contentType );
    DocAction( const DocAction& ) = default;

    void  ready() override = 0;

    auto  SetQuery( const mtc::zmap& ) -> SelfType&;
    auto  AddRoute( const std::string_view& ) -> SelfType&;
    auto  Contents( const std::string_view& ) -> SelfType&;

  protected:
    mtc::zmap           zquery;
    mtc::array_charstr  zroute;
    std::string         cttype;

  };

  template <class SelfType>
  DocAction<SelfType>::DocAction(
    mtc::api<IService> service,
    mtc::api<Response> respond,
    std::string_view   content ): Responder( service, respond ), cttype( content )
  {
  }

  template <class SelfType>
  auto  DocAction<SelfType>::SetQuery( const mtc::zmap& query ) -> SelfType&
  {
    return zquery = query, *(SelfType*)this;
  }

  template <class SelfType>
  auto  DocAction<SelfType>::AddRoute( const std::string_view& arg ) -> SelfType&
  {
    return zroute.emplace_back( arg ), *(SelfType*)this;
  }

  template <class SelfType>
  auto  DocAction<SelfType>::Contents( const std::string_view& arg ) -> SelfType&
  {
    return cttype = std::string( arg ), *(SelfType*)this;
  }

}

# endif // !PALMIRA_RESTAPI_ACTION_HPP_
