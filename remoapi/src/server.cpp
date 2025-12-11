# include "../server.hpp"
# include "../reports.hpp"
# include "loader.hpp"
# include "unpack.hpp"
# include <DeliriX/DOM-load.hpp>
# include <remottp/http-server.hpp>
# include <remottp/src/events.hpp>
# include <remottp/src/server/rest.hpp>
# include <mtc/recursive_shared_mutex.hpp>
# include <condition_variable>

namespace remoapi
{

  class Server final: public palmira::IServer
  {
    mtc::api<palmira::IService> serach;
    http::Server                server;

    std::mutex                  mxwait;
    std::condition_variable     cvwait;
    volatile bool               finish = false;

    void  Start() override;
    void  Stop() override;
    void  Wait() override;

  public:
    Server( mtc::api<palmira::IService> serv, uint16_t port );

  protected:
    implement_lifetime_control

  };

  auto  IsJson( const http::Request& req ) -> bool  {  return req.GetHeaders().get( "Content-Type") == "application/json";  }
  auto  IsDump( const http::Request& req ) -> bool  {  return req.GetHeaders().get( "Content-Type") == "application/octet-stream";  }

  void  OutputHTML( mtc::IByteStream*, http::StatusCode, const char* msgstr );
  void  OutputJSON( mtc::IByteStream*, http::StatusCode, const mtc::zmap& report );

  template <class Args, mtc::api<palmira::IService::IPending> (palmira::IService::*Method)( const Args& )>
  struct ActionCall
  {
    mtc::api<palmira::IService> service;

    void  operator()( mtc::IByteStream* out, const http::Request& req, mtc::IByteStream* src, std::function<bool()> cancel )
    {
      auto  ctType = req.GetHeaders().get( "Content-Type" );
      auto  stream = Unpack( req, src );

      // check if cancel
      if ( cancel() )
        return OutputHTML( out, http::StatusCode::Ok, "request cancelled by user" );

    // parse input args
      try
      {
        Args  args;

        if ( IsJson( req ) )  json::Load( args, req, stream );
          else
        if ( IsDump( req ) )  zmap::Load( args, req, stream );
          else
        throw std::logic_error( "Content-Type is not supported @" __FILE__ ":" LINE_STRING );

        OutputJSON( out, http::StatusCode::Ok, (service->*Method)( args )->Wait() );
      }
      catch ( const mtc::json::parse::error& xp )
      {
        OutputJSON( out, http::StatusCode::Ok, palmira::StatusReport( EINVAL,
          mtc::strprintf( "error parsing request body, line %d: %s", xp.get_json_lineid(), xp.what() ) ) );
      }
      catch ( std::invalid_argument& xp )
      {
        OutputHTML( out, http::StatusCode::BadRequest, xp.what() );
      }
    }
  };

  // Server implementation

  void  Server::Start()
  {
    return server.Start();
  }

  void  Server::Stop()
  {
    server.Stop();
    finish = true;
    cvwait.notify_one();
  }

  void  Server::Wait()
  {
    auto  exlock = mtc::make_unique_lock( mxwait );

    cvwait.wait( exlock, [this](){  return finish;  } );
  }

  Server::Server( mtc::api<palmira::IService> serv, uint16_t port ):
    serach( serv ), server( "0.0.0.0", port )
  {
    server.SetMaxTimeout( 3 * 60 );

    server.RegisterHandler( "/delete", http::Method::GET,   ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );
    server.RegisterHandler( "/remove", http::Method::GET,   ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );
    server.RegisterHandler( "/delete", http::Method::POST,  ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );
    server.RegisterHandler( "/remove", http::Method::POST,  ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );

    server.RegisterHandler( "/update", http::Method::GET,   ActionCall<palmira::UpdateArgs, &palmira::IService::Update>{ serach } );
    server.RegisterHandler( "/update", http::Method::POST,  ActionCall<palmira::UpdateArgs, &palmira::IService::Update>{ serach } );

    server.RegisterHandler( "/insert", http::Method::POST,  ActionCall<palmira::InsertArgs, &palmira::IService::Insert>{ serach } );

  }

  // helpers section

  void  OutputHTML( mtc::IByteStream* output, http::StatusCode status, const char* msgstr )
  {
    Output( output, http::Respond( status, { { "Content-Type", "text/html" } } ),
      mtc::strprintf( "<html>\n"
      "<head><title>%u %s</title></head>\n"
      "<body>%s</body>\n"
      "</html>\n", unsigned(status), http::to_string( status ), msgstr ).c_str() );
  }

  void  OutputJSON( mtc::IByteStream* output, http::StatusCode status, const mtc::zmap& report )
  {
    auto  serial = std::vector<char>();
    mtc::json::Print( &serial, report, mtc::json::print::decorated() );

    Output( output, http::Respond( status, { { "Content-Type", "application/json" } } ),
      serial.data(), serial.size() );
  }

  auto  CreateServer( mtc::api<palmira::IService> serv, uint16_t port ) -> mtc::api<palmira::IServer>
  {
    return new remoapi::Server( serv, port );
  }

}
