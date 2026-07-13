# include "servers.hpp"
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

class hints: protected mtc::zval
{
  struct value: protected zval
  {
    using zval::zval;

    friend class hints;

    value( zval& z ): zval( z ) {}
  };

public:
  hints( const std::initializer_list<std::pair<std::string, value>>& init );

  hints operator [] ( const std::string& key );

  static  value type( const std::string&,
                      const std::initializer_list<std::pair<std::string, value>>& );
  static  value type( const std::initializer_list<std::pair<std::string, value>>& );
  static  value array( mtc::zval::z_type );
  static  value array( const mtc::charstr& );
  static  value array( const mtc::zmap& );
};

hints::hints( const std::initializer_list<std::pair<std::string, value>>& init ): zval( mtc::zmap() )
{
  for ( auto& next: init )
  {
  // check simple type
    switch ( next.second.get_type() )
    {
      case z_byte  :
        get_zmap()->put( next.first, *next.second.get_byte() );
        break;

    // может быть реальный array_charstr, если пустой,
    // или массив стуктур с именем,
    // или явно заданных
      case z_array_charstr:
      {
        auto& arr = *next.second.get_array_charstr();

        if ( arr.empty() )  get_zmap()->put( next.first, next.second );
          else
        if ( arr.size() == 1 )
        {
          auto  map = get_zmap()->get_zmap( arr.front() );

          if ( map != nullptr ) get_zmap()->put( next.first, mtc::array_zmap{ *map } );
            else
          throw std::invalid_argument( "undefined type '" + next.first + "' referenced" );
        }
          else
        throw std::invalid_argument( "invalid type array length" );
        break;
      }
      case z_array_zmap:
      {
        auto& arr = *next.second.get_array_zmap();

        if ( arr.empty() )  get_zmap()->put( next.first, next.second );
          else
        if ( arr.size() == 1 )
        {
          auto& map = arr.front();
          auto  key = map.get_charstr( 1U );

          if ( key != nullptr )
            get_zmap()->set_zmap( *key, map );
        }
          else
        throw std::invalid_argument( "invalid type array length" );
        break;
      }
      case z_zmap:
      {
        auto& map = *next.second.get_zmap();
        auto  key = map.get_charstr( 1U );

        if ( key != nullptr )
          get_zmap()->set_zmap( *key, map );
        break;
      }
      default:
        throw std::invalid_argument( "unexpected hint type" );
    }
  }
}

auto  hints::type( const std::initializer_list<std::pair<std::string, value>>& ht ) -> value
{
  auto  hint = hints( ht );
  return value( hint );
}

auto  hints::type( const std::string& key, const std::initializer_list<std::pair<std::string, value>>& hints ) -> value
{
  return mtc::zmap( *type( hints ).get_zmap(), { { 1U, key } } );
}

auto  hints::array( const mtc::zval::z_type _typ ) -> value
{
  return mtc::array_byte{ _typ };
}

auto  hints::array( const mtc::charstr& _typ ) -> value
{
  return mtc::array_charstr{ _typ };
}

auto  hints::array( const mtc::zmap& _typ ) -> value
{
  return mtc::array_zmap{ _typ };
}

int   main( int argc, char* argv[] )
{
  auto hnts = hints{
      { "version", mtc::zval::z_word64 },
      { "sender", hints::type( "person_info", {
          { "name", mtc::zval::z_widestr },
          { "year", mtc::zval::z_int16 } } ) },
      { "receivers", hints::array("person_info") },
      { "address", hints::type( {
          { "street", mtc::zval::z_charstr },
          { "city", mtc::zval::z_charstr },
          { "zip", mtc::zval::z_int32 } } ) } };

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
    {  server = palmira::servers::GetServers( search, config );  }
  catch ( const std::exception& xp )
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

  std::fprintf(stdout, "The process stopped\n");
  return 0;
}
