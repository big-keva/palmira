# include <service/structo-search.hpp>
# include <watchFs.hpp>
# include <remottp/http-server.hpp>
# include <remottp/src/events.hpp>
# include <structo/queries.hpp>
# include <structo/context/x-contents.hpp>
# include <structo/indexer/layered-contents.hpp>
# include <structo/queries/parser.hpp>
# include <structo/src/indexer/index-layers.hpp>
# include <structo/storage/posix-fs.hpp>
# include <mtc/config.h>
# include <mtc/directory.h>
# include <mtc/threadPool.hpp>
# include <mtc/json.h>
# include <sys/resource.h>
# include <cstdio>

auto  GetJsonQuery( mtc::IByteStream* src ) -> mtc::zmap
{
  auto  jquery = mtc::zmap();

  mtc::json::Parse( src, jquery, mtc::zmap{
    { "query", "widestr" },
    { "first", "int32" },
    { "count", "int32" } } );

  return jquery;
}

auto  GetSkipPaths( const mtc::config& cfg ) -> std::vector<std::string>
{
  return { cfg.get_path( "index_path" ) };
}

auto  BuildService( const mtc::config& cfg ) -> mtc::api<palmira::IService>
{
  auto  ixpath = cfg.get_path( "index_path" );

  if ( ixpath.empty() )
    throw std::invalid_argument( "no 'index_path' parameter found @" __FILE__ ":" LINE_STRING );

  return palmira::StructoService()
    .Set( structo::indexer::layered::Index()
      .Set(
        Open(
          structo::storage::posixFS::StoragePolicies::Open( ixpath ) ) )
      .Create() )
    .Set( structo::context::GetRichContents )
    .Set( structo::context::Processor() )
  .Create();
}

auto  LoadText( const std::string& path ) -> DeliriX::Text
{
  auto  getdoc = DeliriX::Text();
  auto  infile = fopen( path.c_str(), "rt" );

  if ( infile != nullptr )
  {
    char  szline[0x1000];

    while ( fgets( szline, sizeof(szline) - 1, infile ) != nullptr )
      getdoc.AddBlock( szline );

    fclose( infile );
  }
  return getdoc;
}

bool  CheckExt( const std::string& stpath, const std::initializer_list<const char*>& extset )
{
  auto  dotpos = stpath.rfind( '.' );

  if ( dotpos != std::string::npos )
  {
    auto  extstr = stpath.substr( dotpos + 1 );

    for ( auto& ext: extset )
      if ( extstr == ext )
        return true;
  }
  return false;
}

mtc::ThreadPool   indexQueue;

void  IndexDir( mtc::api<palmira::IService> service, const std::string dir, std::atomic_long* counter = nullptr )
{
  auto  thedir = mtc::directory::Open( (!dir.empty() && dir.back() != '/' ? dir + '/' : dir).c_str() );
  auto  rCount = std::atomic_long( 1 );

// increment atomic counter
  if ( counter != nullptr ) ++*counter;
    else counter = &rCount;

  for ( auto dirent = thedir.Get(); dirent.defined(); dirent = thedir.Get() )
    if ( *dirent.string() != '.' )
    {
      auto  stpath = mtc::strprintf( "%s%s", dirent.folder(), dirent.string() );

      if ( dirent.attrib() & mtc::directory::attr_dir )
      {
        IndexDir( service, stpath, counter );
      }
        else
      if ( CheckExt( stpath, { "h", "hpp", "c", "cpp", "cxx", "txt" } ) )
      {
        auto  getdoc = service->Search( palmira::SearchArgs{
          mtc::zmap{
            { "id", stpath } },
          { { "first", int32_t(1) },
            { "count", int32_t(1) } } } )->Wait();

        if ( getdoc.get_word32( "found", 0 ) == 0 )
        {
          auto  insertTask = [stpath, service]()
            {
              auto  intext = LoadText( stpath );

              if ( intext.GetBlocks().size() != 0 )
                service->Insert( { stpath, intext } );
            };
          while ( !indexQueue.Insert( insertTask, std::chrono::seconds( 1 ) ) )
            (void)NULL;
        }
      }
    }
  if ( --*counter == 0 )
    fputs( "recursive directory indexing completed\n", stdout );
}

int   main(int, char**)
{
  auto  config = mtc::config::Open( "/home/keva/dev/palmira/utils/k2-local/config.json" );
  auto  search = BuildService( config.get_config( "delphi" ) );
  auto  ignore = GetSkipPaths( config.get_config( "delphi" ) );
  auto  server = new http::Server( "0.0.0.0", 8080 );
  auto  fwatch = palmira::WatchDir( [&]( unsigned what, const std::string& sz )
    {
      for ( auto& next: ignore )
        if ( sz.length() >= next.length() && next == sz.substr( 0, next.length() ) )
          return;

      fprintf( stdout, "%s: %s\n",
        what == palmira::WatchDir::create_file ? "create" :
        what == palmira::WatchDir::modify_file ? "modify" :
        what == palmira::WatchDir::delete_file ? "delete" : "??????", sz.c_str() );
    } );
  auto  scanIt = std::thread( IndexDir, search, std::string( "/home/keva/" ), nullptr );

  fwatch.AddWatch( "/home/keva/" );

  server->RegisterHandler( "/search", http::Method::POST, [&](
    mtc::IByteStream*     output,
    const http::Request&  refreq,
    mtc::IByteStream*     source,
    std::function<bool()> cancel )
  {
    if ( refreq.GetHeaders().get( "Content-Type" ) == "application/json" )
    {
      auto  zquery = GetJsonQuery( source );
      auto  nfirst = zquery.get_int32( "first", 1 );
      auto  ncount = zquery.get_int32( "count", 10 );
      auto  wquery = zquery.get_widestr( "query", {} );

      if ( wquery.length() != 0 )
      {
        auto  report = search->Search( { structo::queries::ParseQuery( wquery ), mtc::zmap{
          { "first", nfirst },
          { "count", ncount } }, {} } )->Wait();

        http::Output( output, { http::StatusCode::Ok, {
      //    { "Transfer-Encoding", "chunked" },
          { "Access-Control-Allow-Origin", "*" },
          { "Connection", "keep-alive" }/*,
          { "Keep-Alive", "timeout=30, max=1000" }*/ } }, report );
      }
    }
      else
    http::Output( output, { http::StatusCode::BadRequest, { { "Access-Control-Allow-Origin", "*" } } } );
    cancel();
  } );

  server->Start();

  char in[10];
  fgets( in, 10, stdin );

  scanIt.join();

  server->Stop();

  return 0;
}
