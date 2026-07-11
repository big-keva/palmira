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

template <>
inline  std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
  {  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }

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

  auto  IsJson( const http::Request& req ) -> bool
    {  return req.GetHeaders().get( "Content-Type").substr( 0, 16 ) == "application/json";  }
  auto  IsDump( const http::Request& req ) -> bool
    {  return req.GetHeaders().get( "Content-Type").substr( 0, 24 ) == "application/octet-stream";  }
  auto  IsHead( const http::Request& req ) -> bool
    {  return req.GetHeaders().get( "Content-Type") == "" && req.GetMethod() == http::Method::GET;  }

  void  OutputHTML( mtc::IByteStream*, const http::Respond&, const char* msgstr );
  void  OutputJSON( mtc::IByteStream*, const http::Respond&, const mtc::zmap& report );

  template <class Args, mtc::api<palmira::IService::IPending> (palmira::IService::*Method)
    ( const Args&, palmira::IService::NotifyFn )>
  struct ActionCall
  {
    mtc::api<palmira::IService> service;

    void  operator()( mtc::IByteStream* out, const http::Request& req, mtc::IByteStream* src, std::function<bool()> cancel )
    {
      auto  ctType = req.GetHeaders().get( "Content-Type" );
      auto  stream = Inflate( req, src );

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
        if ( IsHead( req ) )  json::Load( args, req, nullptr );
          else
        throw std::logic_error( "Unexpected request method @" __FILE__ ":" LINE_STRING );

        OutputJSON( out, { http::StatusCode::Ok, { { "Access-Control-Allow-Origin", "*" } } },
          (service->*Method)( args, []( const mtc::zmap& ){} )->Wait() );
      }
      catch ( const mtc::json::parse::error& xp )
      {
        OutputJSON( out, { http::StatusCode::Ok, { { "Access-Control-Allow-Origin", "*" } } },
          palmira::StatusReport( EINVAL, mtc::strprintf( "error parsing request body, line %d: %s",
            xp.get_json_lineid(), xp.what() ) ) );
      }
      catch ( std::invalid_argument& xp )
      {
        OutputHTML( out, { http::StatusCode::BadRequest,
          { { "Access-Control-Allow-Origin", "*" } } }, xp.what() );
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

    server.RegisterHandler( "/health", http::Method::GET, [](
      mtc::IByteStream*     output,
      const http::Request&  hthead,
      mtc::IByteStream*     htbody,
      std::function<bool()> cancel )
    {
      (void)hthead;
      (void)htbody;
      (void)cancel;
      OutputJSON( output, { http::StatusCode::Ok, {
        { "Access-Control-Allow-Origin", "*" },
        { "Connection", "keep-alive" } } }, {
        { "status", palmira::Status( 0, "OK" ) } } );
    } );

    server.RegisterHandler( "/delete", http::Method::GET,   ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );
    server.RegisterHandler( "/remove", http::Method::GET,   ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );
    server.RegisterHandler( "/delete", http::Method::POST,  ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );
    server.RegisterHandler( "/remove", http::Method::POST,  ActionCall<palmira::RemoveArgs, &palmira::IService::Remove>{ serach } );

    server.RegisterHandler( "/update", http::Method::GET,   ActionCall<palmira::UpdateArgs, &palmira::IService::Update>{ serach } );
    server.RegisterHandler( "/update", http::Method::POST,  ActionCall<palmira::UpdateArgs, &palmira::IService::Update>{ serach } );

    server.RegisterHandler( "/insert", http::Method::POST,  ActionCall<palmira::InsertArgs, &palmira::IService::Insert>{ serach } );

    server.RegisterHandler( "/search", http::Method::GET,   ActionCall<palmira::SearchArgs, &palmira::IService::Search>{ serach } );
    server.RegisterHandler( "/search", http::Method::POST,  ActionCall<palmira::SearchArgs, &palmira::IService::Search>{ serach } );
  }

  // helpers section

  void  OutputHTML( mtc::IByteStream* output, const http::Respond& result, const char* msgstr )
  {
    Output( output, http::Respond( result, { { "Content-Type", "text/html" } } ),
      mtc::strprintf( "<html>\n"
      "<head><title>%u %s</title></head>\n"
      "<body>%s</body>\n"
      "</html>\n", unsigned(result.GetStatusCode()), http::to_string( result.GetStatusCode() ), msgstr ).c_str() );
  }

  void  OutputJSON( mtc::IByteStream* output, const http::Respond& result, const mtc::zmap& report )
  {
    auto  serial = std::vector<char>();
    auto  extRes = http::Respond( result,
      { { "Content-Type", "application/json" } } );

    mtc::json::Print( &serial, report, mtc::json::print::decorated() );

    Output( output, result, serial.data(), serial.size() );
  }

  auto  CreateServer( mtc::api<palmira::IService> serv, uint16_t port ) -> mtc::api<palmira::IServer>
  {
    return new Server( serv, port );
  }

}
