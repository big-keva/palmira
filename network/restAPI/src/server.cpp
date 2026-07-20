# include "delete-entity.hpp"
# include "update-entity.hpp"
# include "insert-entity.hpp"
# include <server.hpp>
# include <service.hpp>
//# include <reports.hpp>
//# include "loader.hpp"
//# include "unpack.hpp"
//# include <DeliriX/DOM-load.hpp>
# include <uWebSockets/src/App.h>
//# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/threadPool.hpp>
//# include <condition_variable>
# include <mtc/config.h>
//# include <simdjson.h>

template <>
inline  std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
  {  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }


namespace restAPI
{
  using IService = palmira::IService;

  class Server final: public palmira::IServer
  {
    mtc::ThreadPool           thPool;
    uWS::Loop*                pwLoop = nullptr;
    us_listen_socket_t*       listen = nullptr;
    std::shared_ptr<uWS::App> pwMain;
    mtc::api<IService>        search;
    uint16_t                  dwPort;

  public:
    Server( mtc::api<IService> s, uint16_t p ): search( s ), dwPort( p )
    {}

  protected:
    void  Start() override;
    void  Stop() override;
    void  Wait() override;

    implement_lifetime_control
  };
/*
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
*/
  // Server implementation

  void  Server::Start()
  {
      pwMain = std::make_shared<uWS::App>();

//    pwMain->delete( );
//    pwMain->put( );
//    pwMain->post( );
//    pwMain->head( );
   /*
    * DELETE [/{index}]/{id}
    */
    pwMain->del( "/:space/:id", [this]( auto* respond, auto* request )
      {
        auto  action = DeleteEntity( search, restAPI::MakeResponse( respond ) )
          .SetSpace( request->getParameter( 0 ) )
          .SetDocId( request->getParameter( 1 ) );

        respond->onData( [action] ( std::string_view chunk, bool final ) mutable
          {  return action.chunk( chunk, final );  } );
        respond->onAborted( [action]() mutable
          {  return action.abort();  } );
      } );
   /*
    * PATCH [/{index}]/{id}
    */
    pwMain->patch( "/:space/:id", [this]( auto* respond, auto* request )
      {
        auto  action = UpdateEntity( search, restAPI::MakeResponse( respond ) )
          .SetSpace( request->getParameter( 0 ) )
          .SetDocId( request->getParameter( 1 ) );

        respond->onData( [action] ( std::string_view chunk, bool final ) mutable
          {  return action.chunk( chunk, final );  } );
        respond->onAborted( [action]() mutable
          {  return action.abort();  } );
      } );
   /*
    * PUT [/{index}]/{id}
    *
    * Безусловным образом вставить документ 'id' в пространство 'space'.
    */
    pwMain->put( "/:space/:id", [this]( auto* respond, auto* request )
      {
        auto  action = InsertEntity( search, restAPI::MakeResponse( respond ) )
          .SetSpace( request->getParameter( 0 ) )
          .SetDocId( request->getParameter( 1 ) );

        respond->onData( [action] ( std::string_view chunk, bool final ) mutable
          {  return action.chunk( chunk, final );  } );
        respond->onAborted( [action]() mutable
          {  return action.abort();  } );
      } );
# if 0
    pwMain->get( "/:0/_doc/:1", [this]( auto* respond, auto* request )
      {
        auto  qparams = LoadParams( request->getQuery() );
        auto  nextkey = std::string();
        auto  quotate = mtc::zmap();
        auto  bsource = true;
        auto  include = mtc::array_charstr();
        auto  exclude = mtc::array_charstr();

      // parse '_source'
        nextkey = qparams.get_charstr( "_source", "true" );

        if ( nextkey == "true" )  bsource = true;
          else
        if ( nextkey == "false" ) bsource = false;
            else
        if ( !get( include, nextkey ) )
        {
          return elastic::GetEntity( search, elastic::MakeResponse( respond ) ).Error( "400 Bad Request",
            elastic::JsonError( 400, "illegal_argument_exception", "Failed to parse [_source] as boolean" ) );
        }

       /*
        * if _source defined, check the other parameters
        */
        if ( !bsource )
        {
          auto  functor = elastic::GetEntity( search, elastic::MakeResponse( respond ) );

          functor
            .SetIndex( request->getParameter( 0 ) )
            .SetDocId( request->getParameter( 1 ) );

  //      , quotate );
          respond->onData( [functor] ( std::string_view chunk, bool final ) mutable
            {  return functor.chunk( chunk, final );  } );
          respond->onAborted( [functor]() mutable
            {  return functor.abort();  } );
        }

        auto  functor = elastic::GetEntity( search, elastic::MakeResponse( respond ) );

        functor
          .SetIndex( request->getParameter( 0 ) )
          .SetDocId( request->getParameter( 1 ) );

//      , quotate );
        respond->onData( [functor] ( std::string_view chunk, bool final ) mutable
          {  return functor.chunk( chunk, final );  } );
        respond->onAborted( [functor]() mutable
          {  return functor.abort();  } );
      } );
   /*
    * GET /{index}/_mget
    */
    /*
    pwMain->get( "/:0/_mget", [this]( auto* respond, auto* request )
      {
        auto  quotate = mtc::zmap();

        ListParams( request->getQuery(), {
          { "source",             [&]( std::string_view val )
            {  quotate["mode"] = val == "true" ? "source" : "absent";  } },
          { "_source_includes",   [&]( std::string_view val )
            {  quotate["include"] = string_view::cast<mtc::array_charstr>( val );  } },
          { "_source_excludes",   [&]( std::string_view val )
            {  quotate["exclude"] = string_view::cast<mtc::array_charstr>( val );  } } } );

        auto  functor = elastic::GetEntity::Create( search, respond,
          request->getParameter( 0 ), mtc::charstr( request->getParameter( 1 ) ), quotate );

        respond->onData( [functor]( std::string_view chunk, bool final )
          {  return functor.chunk( chunk, final );  } );
        respond->onAborted( [functor]()
          {  return functor.abort();  } );
      } );
    */
  /*  PUT  */
    pwMain->put( "/:0/_doc/:1", [this]( auto* respond, auto* request )
      {
        auto  functor = elastic::SetEntity( search, elastic::MakeResponse( respond ), {
          { "index", request->getParameter( 0 ) },
          { "docid", request->getParameter( 1 ) },
          { "query", LoadParams( request->getQuery() ) } } );

        functor.Set( thPool );

        respond->onData( [functor] ( std::string_view chunk, bool final ) mutable
          {  return functor.chunk( chunk, final );  } );
        respond->onAborted( [functor]() mutable
          {  return functor.abort();  } );
      } );
    pwMain->put( "/:0/_create/:1", [this]( auto* respond, auto* request )
      {
        respond->onAborted( [](){} );
      } );
    pwMain->post( "/:0/_doc", [this]( auto* respond, auto* request )
      {
        auto  functor = elastic::SetEntity( search, elastic::MakeResponse( respond ), {
          { "index", request->getParameter( 0 ) },
          { "docid", "aaa" } } );

        functor
//          .Set( timeout )
          .Set( thPool );

        respond->onData( [functor] ( std::string_view chunk, bool final ) mutable
          {  return functor.chunk( chunk, final );  } );
        respond->onAborted( [functor]() mutable
          {  return functor.abort();  } );
      } );
# endif
    pwMain->listen( dwPort, [this]( auto* listenSocket )
      {
        pwLoop = uWS::Loop::get();
        listen = listenSocket;
      } );
  }

  void  Server::Stop()
  {
    if ( pwLoop != nullptr && listen != nullptr )
    {
      us_listen_socket_close(0, listen );
      pwLoop = nullptr;
      listen = nullptr;
    }
  }

  void  Server::Wait()
  {
    if ( pwMain == nullptr )
      throw std::runtime_error( "Server::Wait: uWebSockets server is not initialized!" );
    return pwMain->run(), pwMain.reset();
  }
/*
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

  */
}

extern "C"  int   CreateServer(
  palmira::IServer**  output,       // target IServer pointer
  palmira::IService*  search,       // cource IService object
  const mtc::config&  config )      // configuration path anchor
{
  auto  dwport = config.get_uint32( "port", unsigned(-1) );

  if ( output == nullptr || search == nullptr )
    return EINVAL;

  if ( dwport == unsigned(-1) )
    return EFAULT;

  (*output = new restAPI::Server( search, dwport ))->Attach();
    return (void)config, 0;
}
