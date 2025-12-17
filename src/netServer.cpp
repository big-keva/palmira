# include "netServer.hpp"
# include <network/grpc-server.hpp>
# include <network/http-server.hpp>
# include <structo/compat.hpp>

namespace palmira
{

  auto  CreateGrpcServer( mtc::api<palmira::IService> srv, mtc::config& cfg ) -> mtc::api<palmira::IServer>
  {
    auto  dwport = cfg.get_int32( "port", -1 );

    if ( dwport > 0 && uint16_t(dwport) == dwport )
      return grpcapi::CreateServer( srv, dwport );

    throw std::invalid_argument( "gRPC 'port' has to be uint16 @" __FILE__ ":" LINE_STRING );
  }

  auto  CreateHttpServer( mtc::api<palmira::IService> srv, mtc::config& cfg ) -> mtc::api<palmira::IServer>
  {
    auto  dwport = cfg.get_int32( "port", -1 );

    if ( dwport > 0 && uint16_t(dwport) == dwport )
      return remoapi::CreateServer( srv, dwport );

    throw std::invalid_argument( "http 'port' has to be uint16 @" __FILE__ ":" LINE_STRING );
  }

  auto  CreateInetServer( mtc::api<palmira::IService> srv, mtc::config& cfg ) -> mtc::api<palmira::IServer>
  {
    auto  getAPI = cfg.to_zmap().get( "api" );

    if ( getAPI != nullptr )
    {
      auto  section = mtc::config();

      if ( getAPI->get_type() == mtc::zval::z_charstr )
      {
        auto  getsec = cfg.to_zmap().get( *getAPI->get_charstr() );

        if ( getsec == nullptr )
        {
          throw std::invalid_argument( mtc::strprintf( "could not find configuration key '%s' @" __FILE__ ":" LINE_STRING,
            getAPI->get_charstr()->c_str() ) );
        }

        if ( getsec->get_type() != mtc::zval::z_zmap )
        {
          throw std::invalid_argument( mtc::strprintf( "'%s' has to point to section @" __FILE__ ":" LINE_STRING,
            getAPI->get_charstr()->c_str() ) );
        }

        section = cfg.get_section( *getAPI->get_charstr() );
      }
        else
      if ( getAPI->get_type() == mtc::zval::z_zmap )
      {
        section = cfg.get_section( "api" );
      }

    // check if empty
      if ( section.empty() )
        throw std::invalid_argument( "empty configuration section pointed by 'api' @" __FILE__ ":" LINE_STRING );

    // check the type
      if ( section.get_charstr( "type" ) == "http" )
        return CreateHttpServer( srv, section );

      if ( section.get_charstr( "type" ) == "gRPC" )
        return CreateGrpcServer( srv, section );

      if ( section.get_charstr( "type" ) == "" )
        throw std::invalid_argument( "'api' section has no 'type' key @" __FILE__ ":" LINE_STRING );

      throw std::invalid_argument( mtc::strprintf( "unknown api type '%s' @" __FILE__ ":" LINE_STRING,
        section.get_charstr( "type" ).c_str() ) );
    }
    throw std::invalid_argument( "'api' key has to point to a section or section name "
      "for network interface @" __FILE__ ":" LINE_STRING );
  }

}
