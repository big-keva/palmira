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
# include <statusReport.hpp>
# include <mtc/fileStream.h>
# include <mtc/exceptions.h>
# include <zlib.h>

template <>
inline  std::string* Serialize( std::string* o, const void* p, size_t l )
{  return &(*o += std::string( (const char*)p, l ));  }

template <>
inline  std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
{  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }

volatile  bool  canContinue = true;

namespace archives
{
  std::list<std::string>    list;
  std::mutex                lock;
  std::condition_variable   wait;
  volatile bool             done = false;
}

namespace requests
{
  std::atomic_long  OK = 0;
  std::atomic_long  error = 0;
  std::atomic_long  fault = 0;
  std::atomic_long  timeout = 0;
  std::atomic_long  processing = 0;
  std::atomic_long  maxrequests = 20;
  std::mutex        locker;
  std::condition_variable waiter;
}

namespace logger
{
  std::mutex        logmtx;
  FILE*             logger;

  void  log( const char* format, ... )
  {
    va_list args;
    va_start( args, format );

    auto  locker = mtc::make_unique_lock( logmtx );
    vfprintf( logger, format, args );
    va_end( args );
  }
}

namespace stats
{
  std::atomic_uint32_t  total_docs = 0;
  std::atomic_uint64_t  total_size = 0;
  std::atomic_uint32_t  last_100_size = 0;

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point last_100_start = start;

  std::mutex  locker;
  std::condition_variable waiter;

  void  Monitor()
  {
    auto  exlock = mtc::make_unique_lock( stats::locker );

    while ( waiter.wait( exlock ), canContinue )
    {
      auto  now = std::chrono::steady_clock::now();
      auto  total_diff = 1.0 * std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
      auto  last_100_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_100_start).count() / 1000.0;

      fprintf( stdout, "%u\tdocs, %lu\tbytes, %6.2f (%6.2f) RPS,\t%6.2f (%6.2f) KbPs\n",
        total_docs.load(), total_size.load(),
        total_docs.load() / total_diff, 100 / last_100_diff,
        total_size.load() / total_diff / 1000.0, last_100_size.load() / last_100_diff / 1000.0 );

      last_100_size = 0;
      last_100_start = now;
    }
  }
}

http::Client    http_client( 4 );
//http::Channel   get_channel = http_client.GetChannel( "83.220.175.72", 57571 );
http::Channel   get_channel = http_client.GetChannel( "0.0.0.0", 57571 );

auto  Deflate( const std::vector<char>& src ) -> std::vector<char>
{
  z_stream          deflate_stream;
  std::vector<char> deflate_buffer;

  deflate_stream.zalloc = Z_NULL;
  deflate_stream.zfree = Z_NULL;
  deflate_stream.opaque = Z_NULL;

  if ( src.empty() )
    return {};

// Инициализируем deflate с уровнем сжатия по умолчанию (Z_DEFAULT_COMPRESSION = 6)
  if ( deflateInit( &deflate_stream, Z_DEFAULT_COMPRESSION ) != Z_OK )
    throw std::runtime_error( "deflate init error" );

// Устанавливаем указатель на входные данные
  deflate_stream.next_in = reinterpret_cast<Bytef*>( const_cast<char*>( src.data() ) );
  deflate_stream.avail_in = static_cast<uInt>( src.size() );

  deflate_buffer.resize( deflateBound( &deflate_stream, src.size() ) );

  // Устанавливаем указатель на выходные данные
  deflate_stream.next_out = reinterpret_cast<Bytef*>(deflate_buffer.data());
  deflate_stream.avail_out = static_cast<uInt>(deflate_buffer.size());

  if ( deflate( &deflate_stream, Z_FINISH ) != Z_STREAM_END )
  {
    deflateEnd( &deflate_stream );
    throw std::runtime_error( "data compression error" );
  }

  deflate_buffer.resize( deflate_stream.total_out );

  deflateEnd( &deflate_stream );

  return deflate_buffer;
}

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

void  WashSocket( mtc::IByteStream* stm )
{
  char  buffer[1024];

  while ( stm != nullptr && stm->Get( buffer, sizeof(buffer) ) > 0 )
    (void)NULL;
}

void  IndexArchive( const std::string& name, const DeliriX::Text& text )
{
  std::vector<char> serial;
  bool              packed = false;

  text.Serialize( &serial );

  try
  {
    serial = std::move( Deflate( serial ) );
    packed = true;
  }
  catch ( ... )
  {}

  auto  locker = mtc::make_unique_lock( requests::locker );
    requests::waiter.wait( locker, [&]{  return !canContinue || requests::processing  < requests::maxrequests; } );

  if ( canContinue )
  {
    auto  thereq = http_client.NewRequest( http::Method::POST, mtc::strprintf( "/insert?id=%s", http::UriEncode( name ).c_str() ) )
      .SetHeaders( { { "Content-Type", "application/octet-stream" } } )
      .SetBody( std::move( serial ) )
      .SetCallback( [name, size = text.GetLength()]( http::ExecStatus status, const http::Respond& res, mtc::api<mtc::IByteStream> stm )
        {
          --requests::processing;

          switch ( status )
          {
            case http::ExecStatus::Ok:
            {
              auto    report = palmira::StatusReport();
              double  elapse;

            // get zmap report
              if ( res.GetHeaders().get( "Content-Type" ) == "application/json" )
              {
                try
                  {  mtc::json::Parse( stm.ptr(), report );  }
                catch ( const mtc::json::parse::error& xp )
                  {  report = palmira::StatusReport( EFAULT, "Could not parse the report" );  }
              }

            // clean socket data
              WashSocket( stm );

            // check if error
              if ( report.status().code() != 0 )
              {
                logger::log( "FAULT\t(%d; %s)\t%s\n",
                  report.status().code(), report.status().info().c_str(), name.c_str() );
                ++requests::fault;
                  return requests::waiter.notify_one();
              }

            // use stats
              logger::log( "OK\t%s\n",
                name.c_str() );
              ++requests::OK;
                stats::total_size += size;
                stats::last_100_size += size;
              if ( ++stats::total_docs % 100 == 0 )
                stats::waiter.notify_one();

            // check indexing timeout and align to optimal request length:
            // - if elapsed time is greater than 3s, decrease the number of parallel requests;
            // - if elapsed time is less than 2s, increase it
              elapse = report.get_zmap( "time", {} ).get_double( "elapsed", -1.0 );

              if ( elapse > 3.0 )
              {
                auto  curmax = requests::maxrequests.load();

                while ( !requests::maxrequests.compare_exchange_strong( curmax, std::max( long(curmax) - 1, 4l ) ) )
                  (void)NULL;
              }
                else
              if ( elapse < 2.0 )
              {
                auto  curmax = requests::maxrequests.load();

                while ( !requests::maxrequests.compare_exchange_strong( curmax, std::min( long(curmax) + 1, 100l ) ) )
                  (void)NULL;
              }
              break;
            }
            case http::ExecStatus::Failed:
            {
              logger::log( "ERROR\t%s\n", name.c_str() );
              ++requests::error;
              break;
            }
            case http::ExecStatus::TimedOut:  default:
            {
              auto  curmax = requests::maxrequests.load();

              while ( !requests::maxrequests.compare_exchange_strong( curmax, std::max( long(curmax) - 1, 4l ) ) )
                (void)NULL;

              logger::log( "TIMEOUT\t%s\n", name.c_str() );
              ++requests::timeout;
              break;
            }
          }
          requests::waiter.notify_one();
        } )
      .SetTimeout( 10.0 );

    if ( packed )
      thereq.GetHeaders()["Content-Encoding"] = "deflate";

    thereq.Start( get_channel );

    ++requests::processing;
  }
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

    IndexArchive( archive, text );
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

  logger::logger = fopen( "log.txt", "wt" );

  for ( unsigned i = 0; i < std::thread::hardware_concurrency(); ++i )
    thrvec.push_back( std::thread( ReadArchives ) );
  thrvec.push_back( std::thread( stats::Monitor ) );

  ListArchives( folder, "*.zip" );

  for ( auto& thread: thrvec )
    thread.join();

  fclose( logger::logger );

  return 0;
}
