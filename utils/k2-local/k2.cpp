# include "../service/delphi-search.hpp"
# include "watch-fs.hpp"
# include "contrib/remottp/server/server.h"
# include "contrib/remottp/common/message.hpp"
# include "contrib/remottp/server/chunks.h"
# include <iostream>
# include <cstdio>
#include <DelphiX/queries.hpp>
#include <DelphiX/context/x-contents.hpp>
#include <DelphiX/indexer/layered-contents.hpp>
#include <DelphiX/indexer/static-contents.hpp>
#include <DelphiX/queries/parser.hpp>
#include <DelphiX/src/indexer/index-layers.hpp>
#include <DelphiX/storage/posix-fs.hpp>
#include <mtc/config.h>
#include <mtc/directory.h>
# include <mtc/json.h>
# include <sys/resource.h>

auto  GetJsonQuery( mtc::IByteStream* src ) -> mtc::zmap
{
  auto  jquery = mtc::zmap();

  mtc::json::Parse( src, jquery, mtc::zmap{
    { "query", "widestr" },
    { "first", "int32" },
    { "count", "int32" } } );

  return jquery;
}

auto  BuildService( const mtc::config& cfg ) -> mtc::api<palmira::IService>
{
  auto  ixpath = cfg.get_path( "index_path" );

  if ( ixpath.empty() )
    throw std::invalid_argument( "no 'index_path' parameter found @" __FILE__ ":" LINE_STRING );

  return palmira::DelphiXService()
    .Set( DelphiX::indexer::layered::Index()
      .Set(
        Open(
          DelphiX::storage::posixFS::StoragePolicies::Open( ixpath ) ) )
      .Create() )
    .Set( DelphiX::context::GetRichContents )
    .Set( DelphiX::context::Processor() )
  .Create();
}

auto  LoadText( const std::string& path ) -> DelphiX::textAPI::Document
{
  auto  getdoc = DelphiX::textAPI::Document();
  auto  infile = fopen( path.c_str(), "rt" );

  if ( infile != nullptr )
  {
    char  szline[0x1000];

    while ( fgets( szline, sizeof(szline) - 1, infile ) != nullptr )
      getdoc.AddString( szline );

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

void  IndexDir( mtc::api<palmira::IService> service, const std::string dir )
{
  auto  thedir = mtc::directory::Open( (!dir.empty() && dir.back() != '/' ? dir + '/' : dir).c_str() );

  for ( auto dirent = thedir.Get(); dirent.defined(); dirent = thedir.Get() )
    if ( *dirent.string() != '.' )
    {
      auto  stpath = mtc::strprintf( "%s%s", dirent.folder(), dirent.string() );

      if ( dirent.attrib() & mtc::directory::attr_dir )
      {
        IndexDir( service, stpath );
      }
        else
      if ( CheckExt( stpath, { "h", "hpp", "c", "cpp", "cxx", "txt" } ) )
      {
        auto  getdoc = service->Search( { mtc::zmap{
          { "id", stpath } }, {
          { "first", int32_t(1) },
          { "count", int32_t(1) } }, {} } );

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
}

int   main(int, char**)
{
  auto  config = mtc::config::Open( "/home/keva/dev/palmira/utils/k2-local/config.json" );
  auto  search = BuildService( config.get_config( "delphi" ) );
  auto  server = new http::Server( "0.0.0.0", 8080 );
  auto  fwatch = palmira::k2_find::WatchDir( [&]( unsigned what, const std::string& sz )
    {
      fprintf( stdout, "%s: %s\n",
        what == palmira::k2_find::WatchDir::create_file ? "create" :
        what == palmira::k2_find::WatchDir::modify_file ? "modify" :
        what == palmira::k2_find::WatchDir::delete_file ? "delete" : "??????", sz.c_str() );
    } );
  auto  scanIt = std::thread( IndexDir, search, std::string( "/home/keva/" ) );

  fwatch.AddWatch( "/home/keva/" );

  server->RegisterHandler( "/search", http::Method::POST, [&](
    mtc::IByteStream*     out,
    const http::Request&  req,
    mtc::IByteStream*     src )
  {
    if ( req.GetHeaders().get( "Content-Type" ) == "application/json" )
    {
      auto  zquery = GetJsonQuery( src );
      auto  nfirst = zquery.get_int32( "first", 1 );
      auto  ncount = zquery.get_int32( "count", 10 );
      auto  wquery = zquery.get_widestr( "query", {} );

      if ( wquery.length() != 0 )
      {
        auto  report = search->Search( { DelphiX::queries::ParseQuery( wquery ), {
          { "first", nfirst },
          { "count", ncount } }, {} } );

        http::OutputResponse( out, { http::StatusCode::Ok, {
      //    { "Transfer-Encoding", "chunked" },
          { "Access-Control-Allow-Origin", "*" } } }, report/*{
          { "error", mtc::zmap{
            { "code", 0 },
            { "info", "OK" } } },
          { "query", wquery },
          { "first", nfirst },
          { "found", 10 },
          { "items", mtc::array_zmap{
            { { "id", "Первый про '" + codepages::widetombcs( codepages::codepage_utf8, wquery ) + "'" },
              { "range", 1.0 } },
            { { "id", "Продолжение о '" + codepages::widetombcs( codepages::codepage_utf8, wquery ) + "'" },
              { "range", 0.9 } } } } }*/ );
      }
    } else http::OutputResponse( out, { http::StatusCode::BadRequest, { { "Access-Control-Allow-Origin", "*" } } } );
  } );

  server->Start();

  char in[10];
  fgets( in, 10, stdin );

  scanIt.join();

  server->Stop();

  return 0;
}
