# include "netServer.hpp"
# include "../service/structo-search.hpp"
# include "../plugins.hpp"
# include "structo/context/x-contents.hpp"
# include "structo/queries.hpp"
# include "structo/indexer/layered-contents.hpp"
# include "structo/storage/posix-fs.hpp"
# include <mtc/sharedLibrary.hpp>
# include <mtc/config.h>
# include <mtc/json.h>
# include <pthread.h>
# include <signal.h>
# include <zlib.h>
# include <system_error>
# include <functional>
# include <thread>

void  BlockSignals()
{
  sigset_t mask;

  sigemptyset( &mask );
  sigaddset( &mask, SIGTERM );
  sigaddset( &mask, SIGQUIT );
  sigaddset( &mask, SIGHUP );

  // Блокируем сигналы для всего процесса
  if ( pthread_sigmask( SIG_BLOCK, &mask, nullptr ) != 0 )
    throw std::system_error( errno, std::system_category(), "Failed to block signals" );
}

std::function<void()> signalFunc;

void  SignalProc( int /*sig*/ )
{
  if ( signalFunc != nullptr )
    signalFunc();
}

template <class ... Exceptions, class Action>
int   Protected( Action action );

template <class Exp, class ... Exceptions, class Action>
int   Protected( Action action )
{
  try
  {  return Protected<Exceptions...>( action );  }
  catch ( const Exp& xp )
  {  return fprintf( stderr, "%s\n", xp.what() ), EFAULT;  }
}

template <class Action>
int   Protected( Action action )
{  return action();  }

template <class ... Exceptions, class Action, class ... Args>
int   Protected( Action action, Args ... args )
{
  return Protected<Exceptions...>( action, args... );
}

int   main( int argc, char* argv[] )
{
  auto  server = mtc::api<palmira::IServer>();
  auto  search = mtc::api<palmira::IService>();
  auto  config = mtc::config();

  if ( argc < 2 )
    return fprintf( stdout, "Usage: %s config.name\n", argv[0] ), EINVAL;

// open the configuration
  try
    {  config = config.Open( argv[1] );  }
  catch ( const mtc::config::error& xp )
    {  return fprintf( stderr, "Config error: %s\n", xp.what() );  }
  catch ( const mtc::json::parse::error& xp )
    {  return fprintf( stderr, "Error parsing config '%s', line %d: %s\n", argv[1], xp.get_json_lineid(), xp.what() );  }
  catch ( ... )
    {  return fprintf( stderr, "Unknwon error opening config\n" );  }

// create search
  try
    {
      auto  getcfg = config.get_section( "service" );

      if ( getcfg.empty() )
        return fprintf( stderr, "Section 'service' not found in configuration file\n" );

      if ( (search = palmira::CreateStructo( getcfg )) == nullptr )
        throw std::logic_error( "unexpected OpenSearch(...) result 'nullptr'" );
    }
  catch ( const std::invalid_argument& xp )
    {  return fprintf( stderr, "Invalid argument: %s\n", xp.what() ), EINVAL;  }

// create server
  try
    {  server = CreateInetServer( search, config );  }
  catch ( const std::invalid_argument& xp )
    {  return fprintf( stderr, "Invalid argument: %s\n", xp.what() ), EINVAL;  }

// install signals handler
  struct sigaction sa;

  memset( &sa, 0, sizeof(sa) );
  sa.sa_handler = SignalProc;
  sigemptyset(  &sa.sa_mask );

  sigaction( SIGINT,  &sa, nullptr );
  sigaction( SIGTERM, &sa, nullptr );
  sigaction( SIGQUIT, &sa, nullptr );

  signalFunc = [server](){  fprintf( stderr, "got stop\n" );  server->Stop();  };

  server->Start();
  server->Wait();

  search->Commit();

  return 0;
}
