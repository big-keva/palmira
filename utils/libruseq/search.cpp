# include <remottp/server.hpp>
# include <remottp/src/events.hpp>
# include <remottp/src/server/rest.hpp>
# include "service/delphi-search.hpp"
# include "structo/context/x-contents.hpp"
# include "structo/queries.hpp"
# include "structo/indexer/layered-contents.hpp"
# include "structo/storage/posix-fs.hpp"
# include "DeliriX/DOM-load.hpp"
# include <mtc/directory.h>
# include <condition_variable>
# include <statusReport.hpp>
# include <vector>
# include <mtc/config.h>
# include <mtc/json.h>

template <>
std::string* Serialize( std::string* o, const void* p, size_t l )
  {  return &(*o += std::string( (const char*)p, l ));  }

template <>
std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
  {  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }

auto  StringTime( const std::chrono::system_clock::time_point& time )
{
  auto  utimer = std::chrono::system_clock::to_time_t( time );
  auto  tvalue = tm{};
  auto  millit = std::chrono::duration_cast<std::chrono::milliseconds>( time.time_since_epoch()
    % std::chrono::seconds(1) ).count();

  localtime_r( &utimer, &tvalue );

  return mtc::strprintf( "%d-%02d-%02d %02d:%02d:%02d.%03d",
    tvalue.tm_year + 1900, tvalue.tm_mon + 1, tvalue.tm_mday,
    tvalue.tm_hour, tvalue.tm_min, tvalue.tm_sec, millit );
}

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

auto  OpenStore( const mtc::config& config ) -> mtc::api<structo::IStorage>
{
  auto  ixpath = config.get_path( "generic_name" );

  if ( ixpath != "" )
    return Open( structo::storage::posixFS::StoragePolicies::Open( ixpath ) );

  throw std::invalid_argument( "neither generic index name nor index policy was found" );
}

auto  OpenIndex( const mtc::config& config ) -> mtc::api<structo::IContentsIndex>
{
  return structo::indexer::layered::Index()
    .Set( OpenStore( config ) )
    .Create();
}

auto  OpenSearch( const mtc::config& config ) -> mtc::api<palmira::IService>
{
  auto  cfgidx = config.get_section( "index" );

  if ( cfgidx.empty() )
    throw std::invalid_argument( "section 'index' not found in configuration file" );

  return palmira::StructoService()
    .Set( OpenIndex( cfgidx ) )
    .Set( structo::context::GetRichContents )
    .Set( structo::context::Processor() )
  .Create();
}

auto  GetJsonInsertArgs( DeliriX::Text& txt, mtc::IByteStream* src ) -> mtc::zmap
{
  auto  jinput = mtc::json::parse::make_source( src );
  auto  reader = mtc::json::parse::reader( jinput );

  // parse json arguments
  return REST::JSON::ParseInput( reader, {
    { "document", [&]( mtc::json::parse::reader& reader )
      {  DeliriX::load_as::Json( &txt, [&](){  return reader.getnext();  } );  } } } );
}

auto  GetDumpInsertArgs( DeliriX::Text& txt, mtc::IByteStream* src ) -> mtc::zmap
{
  if ( txt.FetchFrom( src ) == nullptr )
    throw std::invalid_argument( "application/octet-stream contains invalid data" );
  return {};
}

int   main( int argc, char* argv[] )
{
  auto  server = http::Server();
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
  {  return fprintf( stderr, "Error parsing config JSON, line %d: %s\n", xp.get_json_lineid(), xp.what() );  }
  catch ( ... )
  {  return fprintf( stderr, "Unknwon error opening config\n" );  }

// create search
  try
  {
    auto  getcfg = config.get_section( "service" );

    if ( getcfg.empty() )
      return fprintf( stderr, "Section 'service' not found in configuration file\n" );

    if ( (search = OpenSearch( getcfg )) == nullptr )
      throw std::logic_error( "unexpected OpenSearch(...) result 'nullptr'" );
  }
  catch ( const std::invalid_argument& xp )
  {  return fprintf( stderr, "Invalid argument: %s\n", xp.what() ), EINVAL;  }

// create server
  try
  {
    auto    getcfg = config.get_section( "http" );
    int32_t dwport;
    double  tm_out;

    if ( getcfg.empty() )
      return fprintf( stderr, "Section 'http' not found in configuration file\n" );

    if ( getcfg.to_zmap().get( "port" ) == nullptr )
      throw std::invalid_argument( "undefined 'http->port', expected to be uint16" );

    if ( (dwport = getcfg.get_int32( "port", -1 )) < 0 )
      throw std::invalid_argument( "invalid 'http->port' value" );

    if ( dwport > std::numeric_limits<uint16_t>::max() )
      throw std::invalid_argument( "invalid 'http->port' value" );

    tm_out = getcfg.get_double( "timeout", -1 );

    server = http::Server( "localhost", dwport )
      .SetMaxTimeout( tm_out );
  }
  catch ( const std::invalid_argument& xp )
  {  return fprintf( stderr, "Invalid argument: %s\n", xp.what() ), EINVAL;  }

// configure server
  server.RegisterHandler( "/insert", http::Method::POST,
    [search]( mtc::IByteStream* out, const http::Request& req, mtc::IByteStream* src )
    {
      auto  cnType = req.GetHeaders().get( "Content-Type" );
      auto  jsargs = mtc::zmap();
      auto& params = req.GetUri().parameters();
      auto  pdocid = params.find( "id" );
      auto  sdocid = pdocid != params.end() ? http::UriDecode( pdocid->second ) : std::string();
      auto  intext = DeliriX::Text();
      auto  tstart = std::chrono::system_clock::now();

    // check document body
      if ( src == nullptr )
        return OutputHTML( out, http::StatusCode::BadRequest, "Request POST '/insert' has to body" );

    // get the object depending on the type of contents
      try
      {
        if ( cnType.substr( 0, 16 ) == "application/json" )         jsargs = GetJsonInsertArgs( intext, src );
          else
        if ( cnType.substr( 0, 24 ) == "application/octet-stream" ) jsargs = GetDumpInsertArgs( intext, src );
          else
        return OutputHTML( out, http::StatusCode::BadRequest, "Request POST '/insert' has unknown Content-Type" );
      }
      catch ( const mtc::json::parse::error& xp )
      {
        return OutputJSON( out, http::StatusCode::Ok, palmira::StatusReport( EINVAL,
          mtc::strprintf( "error parsing request body, line %d: %s", xp.get_json_lineid(), xp.what() ) ) );
      }
      catch ( const std::invalid_argument& xp )
      {
        return OutputJSON( out, http::StatusCode::Ok, palmira::StatusReport( EINVAL,
          mtc::strprintf( "error reading request body: %s", xp.what() ) ) );
      }

    // check 'id' parameter
      if ( sdocid.empty() )
      {
        if ( jsargs.get_charstr( "id" ) != nullptr )
          sdocid = *jsargs.get_charstr( "id" );

        if ( sdocid.empty() )
          return OutputHTML( out, http::StatusCode::BadRequest, "Request '/insert' URI has to contain parameter 'id'" );
      }

      auto  finish = std::chrono::system_clock::now();

    // index document
      OutputJSON( out, http::StatusCode::Ok, mtc::zmap(
        search->Insert( { sdocid, intext, jsargs.get_zmap( "metadata", {} ) } ), {
          { "time", mtc::zmap{
            { "started", StringTime( tstart ) },
            { "elapsed", std::chrono::duration_cast<std::chrono::milliseconds>(finish - tstart).count() / 1000.0 } } } } ) );

      fprintf( stdout, "OK\tInsert(%s)\t%6.2f seconds\n", sdocid.c_str(),
        std::chrono::duration_cast<std::chrono::milliseconds>(finish - tstart).count() / 1000.0 );
    } );

  server.RegisterHandler( "/search", http::Method::GET,
    [search]( mtc::IByteStream* out, const http::Request&, mtc::IByteStream* )
    {
      OutputJSON( out, http::StatusCode::Ok, palmira::StatusReport(
        ENOSYS, "Non implemented yet" ) );
    } );

  server.RegisterHandler( "/delete", http::Method::GET,
    [search]( mtc::IByteStream* out, const http::Request& req, mtc::IByteStream* )
    {
      auto& params = req.GetUri().parameters();
      auto  pdocid = params.find( "id" );
      auto  sdocid = pdocid != params.end() ? http::UriDecode( pdocid->second ) : std::string();

    // check loaded parameters
      if ( sdocid.empty() )
        return OutputHTML( out, http::StatusCode::BadRequest, "Request '/delete' URI has to contain parameter 'id'" );

    // index document
      OutputJSON( out, http::StatusCode::Ok,
        search->Remove( { sdocid } ) );
    } );

  server.Start();
    fgetc(stdin);
  server.Stop();
}
