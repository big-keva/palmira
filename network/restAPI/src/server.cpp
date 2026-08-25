#include <reports.hpp>

# include "delete.hpp"
# include "update.hpp"
# include "insert.hpp"
# include "search.hpp"
# include "get-entity.hpp"
# include <server.hpp>
# include <service.hpp>
# include <uWebSockets/src/App.h>
# include <mtc/threadPool.hpp>
# include <mtc/config.h>

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
  * parse arguments and fill the args map
  */
  auto  ParseQuery( std::string_view query ) -> mtc::zmap
  {
    auto  targetData = mtc::zmap();
    auto  skipDelims = []( const char* beg, const char* end )
    {
      while( beg != end && (*beg == '?' || *beg == '&') )
        ++beg;
      return beg;
    };
    auto  parseQuery = [&]( const char* beg, const char* end ) -> mtc::zmap&
    {
      for ( beg = skipDelims( beg, end ); beg < end; beg = skipDelims( beg, end ) )
      {
        auto  key = beg;

        while ( beg != end && *beg != '=' && *beg != '&' )  ++beg;

        if ( beg != key )
        {
          auto  val = targetData.put( { key, size_t(beg - key) } );

          if ( beg != end && *beg++ == '=' )
          {
            auto  org = beg;

            while ( beg != end && *beg != '&' ) ++beg;

            if ( beg != org )
              val->set_charstr( org, beg - org );
          }
        }
      }
      return targetData;
    };

    return query.size() != 0 ? parseQuery( query.data(), query.data() + query.size() ) : targetData;
  }

  // Server implementation

  template <class Action>
  class Dispatch
  {
    mtc::api<IService>  service;
    mtc::ThreadPool*    threads = nullptr;

  public:
    Dispatch( mtc::api<IService> s, mtc::ThreadPool& t ): service( s ), threads( &t ) {}
    Dispatch( mtc::api<IService> s ): service( s ) {}

    template <bool SSL>
    void  operator()( uWS::HttpResponse<SSL>* respond, uWS::HttpRequest* request )
    {
      auto  action = Action( service, restAPI::MakeResponse( respond ) ).
        SetQuery( ParseQuery( request->getQuery() ) ).
        Contents( request->getHeader( "content-type" ) );

      for ( uint16_t u = 0; request->getParameter( u ).size() != 0; ++u )
        action.AddRoute( request->getParameter( u ) );

      if ( threads != nullptr )
        action.SetThreads( *threads );

      respond->onData( [action] ( std::string_view chunk, bool final ) mutable
        {  return action.chunk( chunk, final );  } );
      respond->onAborted( [action]() mutable
        {  return action.abort();  } );
    }
  };

  void  Server::Start()
  {
    pwMain = std::make_shared<uWS::App>();

   /*
    * DELETE      /{space}/{id}
    * GET  /delete/{space}/{id}
    * GET  /remove/{space}/{id}
    * POST /delete
    * POST /remove
    */
    pwMain->del  (        "/:space/:id", Dispatch<Delete>( search, thPool ) );
    pwMain->del  (        "/:space",     Dispatch<Delete>( search, thPool ) );
    pwMain->del  (        "",            Dispatch<Delete>( search, thPool ) );

    pwMain->get  ( "/delete/:space/:id", Dispatch<Delete>( search, thPool ) );
    pwMain->get  ( "/delete/:space",     Dispatch<Delete>( search, thPool ) );
    pwMain->get  ( "/delete",            Dispatch<Delete>( search, thPool ) );

    pwMain->get  ( "/remove/:space/:id", Dispatch<Delete>( search, thPool ) );
    pwMain->get  ( "/remove/:space",     Dispatch<Delete>( search, thPool ) );
    pwMain->get  ( "/remove",            Dispatch<Delete>( search, thPool ) );

    pwMain->post ( "/delete/:space/:id", Dispatch<Delete>( search, thPool ) );
    pwMain->post ( "/delete/:space",     Dispatch<Delete>( search, thPool ) );
    pwMain->post ( "/delete",            Dispatch<Delete>( search, thPool ) );

    pwMain->post ( "/remove/:space/:id", Dispatch<Delete>( search, thPool ) );
    pwMain->post ( "/remove/:space",     Dispatch<Delete>( search, thPool ) );
    pwMain->post ( "/remove",            Dispatch<Delete>( search, thPool ) );

   /*
    * PATCH       /{space}/{id}
    * PATCH       /{space}/{id}
    * POST /update/{space}/{id}
    */
    pwMain->patch( "/:space/:id",        Dispatch<Update>( search, thPool ) );
    pwMain->patch( "/:space",            Dispatch<Update>( search, thPool ) );
    pwMain->patch( "",                   Dispatch<Update>( search, thPool ) );

    pwMain->post ( "/update/:space/:id", Dispatch<Update>( search, thPool ) );
    pwMain->post ( "/update/:space",     Dispatch<Update>( search, thPool ) );
    pwMain->post ( "/update",            Dispatch<Update>( search, thPool ) );

   /*
    * PUT         /{space}/{id}
    * POST /insert/{space}/{id}
    */
    pwMain->put  (        "/:space/:id", Dispatch<Insert>( search, thPool ) );
    pwMain->post ( "/insert/:space/:id", Dispatch<Insert>( search, thPool ) );
    pwMain->post ( "/insert/:space",     Dispatch<Insert>( search, thPool ) );
    pwMain->post ( "/insert",            Dispatch<Insert>( search, thPool ) );

   /*
    * GET       /{space}/{id}
    * POST  /get/{space}/{id}
    * POST  /get/{space}
    * POST  /get
    */
    pwMain->get  (     "/object/:space/:id", Dispatch<GetEntity>( search, thPool ) );
    pwMain->get  (     "/object/:space",     Dispatch<GetEntity>( search, thPool ) );

    pwMain->get  (     "/health",            [this]( auto* res, auto )
      {
        MakeResponse( res )
          ->Instant( "200 OK", palmira::StatusReport( 0, "OK" ) );
      } );

    pwMain->options( "/:action", []( auto* res, auto )
      {
        mtc::json::Print( MakeResponse( res )
          ->WriteStatus( "200 OK" )
          ->WriteHeader( "Content-Type", "application/json; charset=\"utf-8\"" )
          ->WriteHeader( "Access-Control-Allow-Methods", "*" )
          ->WriteHeader( "Access-Control-Allow-Headers", "content-type" ),
            palmira::StatusReport( 0, "OK" ), mtc::json::print::decorated() )
          ->FinishWrite();
      } );

    pwMain->get  ( "/get/:space/:id", Dispatch<GetEntity>( search, thPool ) );
    pwMain->get  ( "/get/:space",     Dispatch<GetEntity>( search, thPool ) );
    pwMain->get  ( "/get",            Dispatch<GetEntity>( search, thPool ) );

    pwMain->post ( "/get/:space/:id", Dispatch<GetEntity>( search, thPool ) );
    pwMain->post ( "/get/:space",     Dispatch<GetEntity>( search, thPool ) );
    pwMain->post ( "/get",            Dispatch<GetEntity>( search, thPool ) );

   /*
    * GET   /search/{space}
    * GET   /search
    * POST  /search/{space}
    * POST  /search
    */
    pwMain->get  ( "/search/:space",  Dispatch<Search>( search, thPool ) );
    pwMain->get  ( "/search",         Dispatch<Search>( search, thPool ) );

    pwMain->post ( "/search/:space",  Dispatch<Search>( search, thPool ) );
    pwMain->post ( "/search",         Dispatch<Search>( search, thPool ) );

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
