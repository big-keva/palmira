# include <service/structo-search.hpp>
# include <structo/context/x-contents.hpp>
# include <DeliriX/DOM-text.hpp>
# include <DeliriX/archive.hpp>
# include <DeliriX/formats.hpp>
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/byteBuffer.h>
# include <mtc/directory.h>
# include <mtc/json.h>
# include <condition_variable>
# include <vector>
# include <list>
#include <DeliriX/DOM-dump.hpp>

std::atomic_llong totalBytes = 0;
std::atomic_long  totalBooks = 0;

volatile bool canContinue = true;
volatile bool noMoreFiles = false;

template <class Value, size_t Limit>
class Collector
{
  std::list<Value>        list;
  std::mutex              lock;
  std::condition_variable wait;
public:
  void  Put( Value&& v )
  {
    auto  exLock = mtc::make_unique_lock( lock );

    wait.wait( exLock, [&](){  return list.size() < Limit || !canContinue;  } );

    list.push_back( std::forward<Value>( v ) );

    wait.notify_all();
  }
  bool  Get( Value& v )
  {
    auto  exLock = mtc::make_unique_lock( lock );

    for ( ; ; )
    {
      wait.wait_for( exLock, std::chrono::seconds( 1 ),
        [&]{  return !canContinue || !list.empty() || noMoreFiles;  } );

      if ( !canContinue )
        return false;
      if ( list.empty() )
      {
        if ( noMoreFiles )
          return false;
        continue;
      }
      v = std::move( list.front() );
        list.pop_front();
      wait.notify_all();
        return true;
    }
  }
};

Collector<std::string, 0x100>                           archives;
Collector<std::pair<std::string, DeliriX::Text>, 0x10>  fb2Texts;

mtc::api<palmira::IService> libRusEq;

auto  LoadFile( const std::string& path ) -> mtc::api<const mtc::IByteBuffer>
{
  std::vector<char> load;
  char              buff[1024 * 0x40];
  auto              file = fopen( path.c_str(), "rb" );

  if ( file == nullptr )
    throw std::runtime_error( "Failed to open file '" + path + "'" );

  for ( auto read = fread( buff, 1, sizeof(buff), file ); read > 0; read = fread( buff, 1, sizeof(buff), file ) )
    load.insert( load.end(), buff, buff + read );

  fclose( file );

  return CreateByteBuffer( load.data(), load.size(), mtc::enable_exceptions ).ptr();
}

void  ReadZipped( const mtc::api<const mtc::IByteBuffer>& buf, const std::string& str )
{
  auto  archive = DeliriX::OpenZip( buf );

  // check if zip
  if ( archive != nullptr )
  {
    for ( auto entry = archive->ReadDir(); entry != nullptr; entry = archive->ReadDir() )
    {
      auto  pslash = str.find_last_of( "\\/" );
      auto  realId = (pslash != str.npos ? str.substr( 0, pslash + 1 ) : "") + entry->GetName();

      ReadZipped( entry->GetFile(), realId );
    }
  }
    else
  // not a zip, it is fb2
  {
    DeliriX::Text thedoc;

    try
    {
      DeliriX::ParseFB2( &thedoc, buf );
      fb2Texts.Put( std::move( std::make_pair( std::move( str ), std::move( thedoc ) ) ) );
    }
    catch ( const std::exception& xp )
    {
      fprintf( stderr, "%s\n", xp.what() );
    }
  }
}

void  ParseTexts()
{
  pthread_setname_np( pthread_self(), "IndexTexts" );

  for ( ; ; )
  {
    auto  sourceId = std::string();

    if ( !archives.Get( sourceId ) )
      break;

    try
    {
      ReadZipped( LoadFile( sourceId ), sourceId );
    }
    catch ( const std::exception& xp )
    {
      fprintf( stderr, "%s\n", xp.what() );
    }
  }
}

void  IndexTexts()
{
  for ( ; ; )
  {
    auto  next = std::pair<std::string, DeliriX::Text>();

    if ( fb2Texts.Get( next ) )
    {
      auto  ptitle = next.second.FindFirstTag( "book-title" );
      auto  author = next.second.FindFirstTag( "author" );
      auto  zmdata = mtc::zmap();

    // get book title
      if ( ptitle != nullptr && !ptitle->GetBlocks().empty() )
      {
        auto  ttlStr = ptitle->GetBlocks().front().GetWideStr();

        if ( !ttlStr.empty() )
          zmdata["title"] = ttlStr;
      }

    // get book author
      if ( author != nullptr && !author->GetBlocks().empty() )
      {
        auto  p_name = author->FindFirstTag( "first-name" );
        auto  p_surn = author->FindFirstTag( "last-name" );
        auto  p_nick = author->FindFirstTag( "nickname" );
        auto  s_name = mtc::widestr();

        if ( p_surn != nullptr && !p_surn->GetBlocks().empty() )
        {
          s_name = p_surn->GetBlocks().front().GetWideStr();

          if ( p_name != nullptr && !p_name->GetBlocks().empty() )
            s_name = mtc::widestr( p_name->GetBlocks().front().GetWideStr() ) + u' ' + s_name;
        }
          else
        if ( p_nick != nullptr && !p_nick->GetBlocks().empty() )
        {
          s_name = p_nick->GetBlocks().front().GetWideStr();
        }
        zmdata["author"] = s_name;
      }

      libRusEq->Insert( { next.first, next.second, zmdata }, [&, length = next.second.GetLength()]( const mtc::zmap& )
        {
          auto  nBooks = totalBooks.load();
          auto  nBytes = totalBytes.load();

          while ( !totalBytes.compare_exchange_strong( nBytes, nBytes + length ) )
            (void)NULL;
          while ( !totalBooks.compare_exchange_strong( nBooks, nBooks + 1 ) )
            (void)NULL;

          if ( (nBooks % 100) == 0 )
            fprintf( stdout, "%ld books,\t%lld Mb\n", nBooks, nBytes / 1024 / 1024 );
        } )->Wait();
    } else break;
  }
}

# include "structo/storage/posix-fs.hpp"
# include "structo/indexer/layered-contents.hpp"

void  ListFiles( mtc::directory folder )
{
  for ( auto dirent = folder.Get(); canContinue && dirent.defined(); dirent = folder.Get() )
    if ( strcmp( dirent.string(), "." ) != 0 && strcmp( dirent.string(), ".." ) != 0 )
    {
      auto  stnext = mtc::strprintf( "%s%s", dirent.folder(), dirent.string() );

      if ( dirent.attrib() & mtc::directory::attr_dir )
      {
        ListFiles( mtc::directory::Open( (stnext + "/").c_str() ) );
      }
        else
      {
        archives.Put( std::move( stnext ) );
      }
    }
}

int   main( int argc, char* argv[] )
{
  auto  folder = "/media/keva/KINGSTON/Libruks/Архивы Либрусек/";
  auto  diread = mtc::directory::Open( folder, mtc::directory::attr_any );
  auto  runthr = std::vector<std::thread>();
  auto  config = mtc::config();

  if ( argc < 2 )
    return fprintf( stdout, "Usage: %s config.name\n", argv[0] ), EINVAL;

  if ( !diread.defined() )
    return fprintf( stderr, "could not open directory '%s'\n", folder ), ENOENT;

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

      if ( (libRusEq = palmira::CreateStructo( getcfg )) == nullptr )
        throw std::logic_error( "unexpected OpenSearch(...) result 'nullptr'" );
    }
  catch ( const std::invalid_argument& xp )
    {  return fprintf( stderr, "Invalid argument: %s\n", xp.what() ), EINVAL;  }

// start parser threads
  for ( size_t i = 0; i < std::thread::hardware_concurrency(); ++i )
    runthr.push_back( std::thread( ParseTexts ) );
  for ( size_t i = 0; i < std::thread::hardware_concurrency(); ++i )
    runthr.push_back( std::thread( IndexTexts ) );
/*
  runthr.push_back( std::thread( [&]()
    {
      auto  globalStart = std::chrono::steady_clock::now();
      auto  momentStart = globalStart;
      auto  momentBooks = 0U;
      auto  momentBytes = 0ULL;

      while ( canContinue && !noMoreBooks )
      {
        std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

        auto  momentTime = std::chrono::steady_clock::now();
        auto  globalSecs = std::chrono::duration_cast<std::chrono::milliseconds>( momentTime - globalStart ).count() / 1000.0;
        auto  momentSecs = std::chrono::duration_cast<std::chrono::milliseconds>( momentTime - momentStart ).count() / 1000.0;
        auto  booksLoaded = totalBooks.load();
        auto  bytesLoaded = totalBytes.load();
        auto  diffBooks = booksLoaded - momentBooks;  momentBooks = booksLoaded;
        auto  diffBytes = bytesLoaded - momentBytes;  momentBytes = bytesLoaded;

        fprintf( stdout, "%u books, %uMb, %u(%u)RPS, %u(%u)MbPS\n",
          unsigned(booksLoaded),
          unsigned(bytesLoaded / 1024 / 1024),
          unsigned(booksLoaded / globalSecs), unsigned(diffBooks / momentSecs),
          unsigned(bytesLoaded / globalSecs / 1024 / 1024), unsigned(diffBytes / momentSecs / 1024 / 1024) );

        momentBooks = totalBooks.load();
        momentBytes = totalBytes.load();
        momentStart = momentTime;
      }
      fprintf( stderr, "monitor finished\n" );
    } ) );
*/

// directory reading thread as main
  ListFiles( diread );
    noMoreFiles = true;
  ParseTexts();
  IndexTexts();

  canContinue = false;

  for ( auto& th: runthr)
    th.join();

  return 0;
}