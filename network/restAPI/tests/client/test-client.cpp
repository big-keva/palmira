# include <reports.hpp>
# include <uWebSockets/src/App.h>
# include <service.hpp>
# include <mtc/config.h>
# include <mtc/json.h>
# include <mtc/test-it-easy.hpp>

// Глобальный флаг для управления временем жизни тестового сервера
std::atomic<bool>   g_serverRuns = true;
uWS::Loop*          g_serverLoop = nullptr;
us_listen_socket_t* g_listenSock = nullptr;

void  DebugServer()
{
  uWS::App().post( "/insert", []( auto* res, auto* req )
  {
    fputs( "===== HEAD =====\n", stdout );
    {
      for ( auto header: *req )
        fprintf( stdout, "%s: %s\n", std::string( header.first ).c_str(), std::string( header.second ).c_str() );
    }

    res->onData([res]( std::string_view bytes, bool )
    {
      std::string serial;

      fputs( "===== BODY =====\n", stdout );
        fwrite( bytes.data(), 1, bytes.size(), stdout );

      res->end( *mtc::json::Print( &serial, palmira::StatusReport( 0, "OK" ),
        mtc::json::print::decorated() ) );
    });
    res->onAborted( [res](){} );
  } )
  .listen( 54321, []( auto* listen_socket )
  {
    g_serverLoop = uWS::Loop::get(); // Сохраняем указатель на цикл сервера
    g_listenSock = listen_socket;
  }).run();
}

using namespace palmira;

extern "C"
{
  int   CreateClient( IService** output, const mtc::config& config );
}

int   main()
{
  mtc::api<IService>  client;
  std::thread         server( DebugServer );

  // Даем серверу немного времени, чтобы инициализировать сокет и начать listen
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  CreateClient( client, {
    { "addr", "http://127.0.0.1:54321" } } );

  client->Insert( {
    "docid",
    DeliriX::Text{
      { "title", { "document title" } },
      { "body", { "hello world!" } } },
    { { "title", "название документа" } },
    4,
    "$doc.version < 5" }, []( const mtc::zmap& res )
  {
    fputs( "\n" "===== ANSWER =====\n", stdout );

    fputs( "\n", mtc::json::Print( stdout, res, mtc::json::print::decorated() ) );
  })->Wait();

  client = nullptr;

  g_serverLoop->defer( []()
    {
      if ( g_listenSock != nullptr )
      {
        us_listen_socket_close(0, g_listenSock);
        g_listenSock = nullptr;
      }
    });

  server.join();

  return 0;
}
