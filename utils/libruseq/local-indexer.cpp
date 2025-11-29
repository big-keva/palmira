# include "service/delphi-search.hpp"
# include "DeliriX/DOM-text.hpp"
# include "structo/context/x-contents.hpp"
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/directory.h>
# include <condition_variable>
# include <vector>
# include <list>
#include <DeliriX/archive.hpp>
#include <DeliriX/formats.hpp>
#include <DelphiX/queries.hpp>
#include <DelphiX/queries/parser.hpp>
#include <mtc/byteBuffer.h>
#include <mtc/json.h>

std::atomic_llong         totalBytes = 0;
std::atomic_long          totalBooks = 0;

const size_t              MaxArcives = 0x10;
const size_t              MaxDomText = 0xc;
const size_t              MaxIndexer = 0xa;

std::list<std::string>    archives;
std::mutex                archLock;
std::condition_variable   archWait;

std::list<std::pair<std::string, DeliriX::Text>>
                          textList;
std::mutex                textLock;
std::condition_variable   textWait;

mtc::api<palmira::IService> libRusEq;

volatile bool             canContinue = true;
volatile bool             noMoreFiles = false;
volatile bool             noMoreBooks = false;

auto  LoadFile( const std::string& path ) -> mtc::api<const mtc::IByteBuffer>
{
  std::vector<char> load;
  char              buff[1024 * 0x40];
  auto              file = fopen( path.c_str(), "rb" );

  for ( auto read = fread( buff, 1, sizeof(buff), file ); read > 0; read = fread( buff, 1, sizeof(buff), file ) )
    load.insert( load.end(), buff, buff + read );

  fclose( file );

  return mtc::CreateByteBuffer( load.data(), load.size(), mtc::enable_exceptions ).ptr();
}

void  InsertText( const std::string& id, const DeliriX::Text& text )
{
  totalBytes += text.GetLength();

  if ( ++totalBooks > 1000)//0 )
  {
    canContinue = false;
    archWait.notify_all();
    textWait.notify_all();
  }

  libRusEq->Insert( { id, text, {} } );
}

void  IndexTexts()
{
  pthread_setname_np( pthread_self(), "IndexTexts" );

/*  try
  {*/
    while ( canContinue && !noMoreBooks )
    {
      auto  waitLock = mtc::make_unique_lock( textLock );
      auto  sourceId = std::string();
      auto  document = DeliriX::Text();

    // get document text
      textWait.wait( waitLock, [&]()
        {  return !canContinue || noMoreBooks || !textList.empty();  } );

      if ( textList.empty() )
        continue;

      std::tie( sourceId, document ) = std::move( textList.front() );
        textList.pop_front();
        textWait.notify_all();
      waitLock.unlock();

      InsertText( sourceId, document );
    }
/*  }
  catch ( ... )
  {
    fprintf( stderr, "indexer exception\n" );
  }*/
//  fprintf( stderr, "indexer finished\n" );
}

void  ReadZipped( const mtc::api<const mtc::IByteBuffer>& buf, const std::string& str )
{
  auto  archive = DeliriX::OpenZip( buf );

  // check if zip
  if ( archive != nullptr )
  {
    for ( auto entry = archive->ReadDir(); entry != nullptr; entry = archive->ReadDir() )
    {
      ReadZipped( entry->GetFile(), str + '#' + entry->GetName() );
    }
  }
    else
  // not a zip, it is fb2
  {
    DeliriX::Text thedoc;
    auto          locker = mtc::make_unique_lock( textLock, std::defer_lock );

    DeliriX::ParseFB2( &thedoc, buf );

    locker.lock();

    if ( textWait.wait( locker, [&](){  return !canContinue || textList.size() < MaxDomText;  } ), canContinue )
      textList.push_back( { str, std::move( thedoc ) } );

    textWait.notify_all();
  }
}

void  ParseBooks()
{
  pthread_setname_np( pthread_self(), "ParseBooks" );

  while ( canContinue && !noMoreFiles )
  {
    try
    {
      auto  waitLock = mtc::make_unique_lock( archLock );
      auto  archPath = std::string();
      auto  bookBuff = mtc::api<const mtc::IByteBuffer>();

      archWait.wait( waitLock, [&]()
        {  return !canContinue || noMoreFiles || !archives.empty();  } );

      if ( archives.empty() )
        continue;

      archPath = std::move( archives.front() );
        archives.pop_front();
        archWait.notify_all();
      archLock.unlock();

      bookBuff = LoadFile( archPath );

      ReadZipped( bookBuff, archPath );
    }
    catch ( const std::exception& xp )
    {
      fprintf( stderr, "%s\n", xp.what() );
    }
    catch ( ... )
    {
      fprintf( stderr, "parser exception\n" );
    }
  }
  noMoreBooks = true;
}

# include "structo/storage/posix-fs.hpp"
# include "structo/indexer/layered-contents.hpp"

auto  GetSearchService() -> mtc::api<palmira::IService>
{
  auto  spolicy = DelphiX::storage::posixFS::StoragePolicies::Open( "/home/keva/dev/palmira/!test" );
  auto  storage = DelphiX::storage::posixFS::Open( spolicy );
  auto  layered = DelphiX::indexer::layered::Index()
    .Set( storage )
    .Set( DelphiX::indexer::dynamic::Settings()
      .SetMaxEntities( 1024 )
      .SetMaxAllocate( 256 * 1024 * 1024 ) )
    .Create();

  return palmira::DelphiXService()
    .Set( layered )
    .Set( DelphiX::context::GetMiniContents )
    .Create();
}

int   main()
{
  auto  folder = "/media/keva/KINGSTON/Libruks/Архивы Либрусек/";
  auto  diread = mtc::directory::Open( folder, mtc::directory::attr_file );
  auto  runthr = std::vector<std::thread>();

  if ( !diread.defined() )
    return fprintf( stderr, "could not open directory '%s'\n", folder ), ENOENT;

// create search service
  libRusEq = GetSearchService();

// start parser threads
  for ( size_t i = 0; i < MaxDomText; ++i )
    runthr.push_back( std::thread( ParseBooks ) );
  for ( size_t i = 0; i < MaxIndexer; ++i )
    runthr.push_back( std::thread( IndexTexts ) );
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

// directory reading thread as main
  runthr.push_back( std::thread( [&]()
  {
    for ( auto dirent = diread.Get(); canContinue && dirent.defined(); dirent = diread.Get() )
    {
      auto  waitLock = mtc::make_unique_lock( archLock );

      archWait.wait( waitLock, [&]()
        {  return !canContinue || archives.size() <= MaxArcives;  } );

      if ( canContinue )
      {
        archives.push_back( mtc::strprintf( "%s%s", dirent.folder(), dirent.string() ) );
        archWait.notify_one();
      }
    }

    noMoreFiles = true;
    canContinue = false;
    archWait.notify_all();
    textWait.notify_all();
  } ) );

  IndexTexts();

  for ( auto& th: runthr)
    th.join();

  return 0;
}