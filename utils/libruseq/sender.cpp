# include <DeliriX/DOM-text.hpp>
# include <DeliriX/archive.hpp>
# include <DeliriX/formats.hpp>
# include <remottp/client.hpp>
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/directory.h>
# include <mtc/json.h>
# include <condition_variable>
# include <vector>
# include <list>
# include <mtc/fileStream.h>
# include <mtc/exceptions.h>

template <>
inline  std::string* Serialize( std::string* o, const void* p, size_t l )
{  return &(*o += std::string( (const char*)p, l ));  }

template <>
inline  std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
{  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }

namespace archives
{
  std::list<std::string>    list;
  std::mutex                lock;
  std::condition_variable   wait;
  volatile bool             done = false;
}

namespace requests
{
  std::atomic_long  sent = 0;
  std::atomic_long  OK = 0;
  std::atomic_long  fault = 0;
  std::atomic_long  timeout = 0;
  std::mutex        locker;
  std::condition_variable waiter;

  std::mutex        logmtx;
  FILE*             logger;
}


volatile  bool  canContinue = true;

http::Client    http_client;
http::Channel   get_channel = http_client.GetChannel( "127.0.0.1", 57571 );

/*
 * Рекурсивный сканер каталога и построения списка архивов для дальнейшего разбора
 * в качестве fb2
 */
void  ListArchives( const char* path, const char* mask )
{
  auto  reader = mtc::directory::Open( path );

  if ( reader.defined() )
    for ( auto dirent = reader.Get(); canContinue && dirent.defined(); dirent = reader.Get() )
      if ( *dirent.string() != '.' )
      {
        auto  fnfull = mtc::strprintf( "%s%s", dirent.folder(), dirent.string() );

        if ( dirent.attrib() & mtc::directory::attr_dir )
        {
          ListArchives( (fnfull + "/").c_str(), mask );
        }
          else
        if ( fnmatch( mask, fnfull.c_str(), 0 ) == 0 )
        {
          auto  lock = mtc::make_unique_lock( archives::lock );

          archives::wait.wait( lock, []{  return !canContinue || archives::list.size() < 100;  } );

          if ( !canContinue )
            return;

          archives::list.push_back( fnfull );
            archives::wait.notify_all();
        }
      }
}

void  StoreArchive( const std::string& name, const DeliriX::Text& text )
{
  std::vector<char> serial;

  text.Serialize( &serial );

  auto  locker = mtc::make_unique_lock( requests::locker );

  requests::waiter.wait( locker, [&]{  return !canContinue || (requests::sent - requests::OK - requests::fault - requests::timeout) < 20; } );

  auto  r = http_client.NewRequest( http::Method::POST, mtc::strprintf( "/insert?id=%s", http::UriEncode( name ).c_str() ) )
    .SetHeaders( { { "Content-Type", "application/octet-stream" } } )
    .SetBody( std::move( serial ) )
    .SetCallback( [name]( http::ExecStatus status, const http::Respond& res, mtc::api<mtc::IByteStream> stm )
      {
        switch ( status )
        {
          case http::ExecStatus::Ok:
          {
            char  buffer[1024];
            int   cbread;

            while ( (cbread = stm->Get( buffer, sizeof(buffer) )) > 0 )
              (void)NULL;
            ++requests::OK;
              break;
          }
          case http::ExecStatus::Failed:
          {
            mtc::interlocked( mtc::make_unique_lock( requests::logmtx ), [&]()
              {  fprintf( requests::logger, "%s\tFAULT\n", name.c_str() );  } );
            ++requests::fault;
              break;
          }
          case http::ExecStatus::TimedOut:  default:
          {
            mtc::interlocked( mtc::make_unique_lock( requests::logmtx ), [&]()
              {  fprintf( requests::logger, "%s\tTIMEOUT\n", name.c_str() );  } );
            ++requests::timeout;
                break;
          }
        }
        requests::waiter.notify_one();
      } )
    .SetTimeout( 5.0 )
    .Start( get_channel );

  ++requests::sent;
  /*
    r.wait();

    if ( r.get().status == http::ExecStatus::Ok )
    {
      fprintf( stdout, "Succeeded: %u %s\n",
        unsigned( r.get().report.GetStatusCode() ), to_string( r.get().report.GetStatusCode() ) );

      for ( auto stream = r.get().stream; stream != nullptr; stream = nullptr )
      {
        char  buffer[1024];
        int   cbread;

        while ( (cbread = stream->Get( buffer, sizeof(buffer) )) > 0 )
          fwrite( buffer, cbread, 1, stdout );
      }
    }
      else
    {
      fprintf( stdout, "%s\n",
        r.get().status == http::ExecStatus::Failed ? "Failed" :
        r.get().status == http::ExecStatus::Invalid ? "Invalid" :
        r.get().status == http::ExecStatus::TimedOut ? "TimedOut" : "Undefined" );
    }
  */
}

void  ParseArchive( const std::string& archive )
{
  auto  infile = mtc::api<mtc::IFileStream>();
  auto  mapped = mtc::api<const mtc::IByteBuffer>();
  auto  zipped = mtc::api<DeliriX::IArchive>();
  auto  zentry = mtc::api<DeliriX::IArchive::IEntry>();
  auto  buffer = mtc::api<const mtc::IByteBuffer>();

// get file data
  try
  {
    infile = OpenFileStream( archive.c_str(), O_RDONLY, mtc::enable_exceptions );
    mapped = infile->MemMap( 0, infile->Size() );
  }
  catch ( const mtc::file_error& xp )
  {  return (void)fprintf( stderr, "could not open file '%s', error '%s'\n", archive.c_str(), xp.what() );  }
  catch ( ... )
  {  return (void)fprintf( stderr, "unknown exception white opening or mapping file '%s'\n", archive.c_str() );  }

// open zip archive
// check if contains an fb2 file
  try
  {
    zipped = DeliriX::OpenZip( mapped );

    if ( (zentry = zipped->ReadDir()) == nullptr )
      throw std::invalid_argument( "archive does not contain entries" );

    if ( fnmatch( "*.fb2", zentry->GetName().c_str(), 0 ) != 0 )
      throw std::invalid_argument( "archive does not contain .fb2 entries" );

    buffer = zentry->GetFile();
  }
  catch ( const std::invalid_argument& xp )
  {  return (void)fprintf( stderr, "could not open zip archive '%s', error '%s'\n", archive.c_str(), xp.what() );  }
  catch ( ... )
  {  return (void)fprintf( stderr, "unknown exception white opening zip archive '%s'\n", archive.c_str() );  }

  zentry = nullptr;
  zipped = nullptr;
  mapped = nullptr;
  infile = nullptr;

// parse .fb2 document
  if ( buffer != nullptr )
  {
    DeliriX::Text text;

    try
    {
      ParseFB2( &text, buffer );
    }
    catch ( ... )
    {  return (void)fprintf( stderr, "unknown exception parsing fb2 from '%s'\n", archive.c_str() );  }

    StoreArchive( archive, text );
  }
}

/*
 * Читатель архивов и разборщик fb2
 */
void  ReadArchives()
{
  while ( canContinue )
  {
    auto  lock = mtc::make_unique_lock( archives::lock );

    archives::wait.wait( lock, []{  return !canContinue || archives::done || !archives::list.empty();  } );

    if ( !canContinue )
      break;

    if ( !archives::list.empty() )
    {
    // get next name
      auto  fnfull = std::move( archives::list.front() );
        archives::list.pop_front();
      lock.unlock();
        archives::wait.notify_all();

      ParseArchive( fnfull );
    }
      else
    if ( archives::done ) break;
  }
}

int   main()
{
  auto  folder = "/media/keva/KINGSTON/Libruks/Архивы Либрусек/";
  auto  thrvec = std::vector<std::thread>();

  requests::logger = fopen( "faults.txt", "wt" );

  for ( unsigned i = 0; i < std::thread::hardware_concurrency(); ++i )
    thrvec.push_back( std::thread( ReadArchives ) );

  ListArchives( folder, "*.zip" );

  for ( auto& thread: thrvec )
    thread.join();

  fclose( requests::logger );

  return 0;
}
