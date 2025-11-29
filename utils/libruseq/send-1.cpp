# include <DeliriX/DOM-text.hpp>
# include <DeliriX/archive.hpp>
# include <DeliriX/formats.hpp>
# include <remottp/client.hpp>
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/directory.h>
# include <mtc/json.h>
# include <condition_variable>
#include <fstream>
# include <vector>
# include <list>
# include <mtc/fileStream.h>
# include <mtc/exceptions.h>

template <>
std::string* Serialize( std::string* o, const void* p, size_t l )
  {  return &(*o += std::string( (const char*)p, l ));  }

template <>
std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
  {  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }

http::Client    http_client;
http::Channel   get_channel = http_client.GetChannel( "127.0.0.1", 57571 );

void  StoreArchive( const std::string& name, const DeliriX::Text& text )
{
  std::vector<char> serial;

  text.Serialize( &serial );

  auto  r = http_client.NewRequest( http::Method::POST, mtc::strprintf( "/insert?id=%s", http::UriEncode( name ).c_str() ) )
    .SetBody( std::move( serial ) )
    .SetTimeout( 5.0 )
    .SetCallback( []( http::ExecStatus status, const http::Respond& resp, mtc::api<mtc::IByteStream> stm )
      {
        if ( status == http::ExecStatus::Ok )
        {
          fprintf( stdout, "Succeeded: %u %s\n",
            unsigned( resp.GetStatusCode() ), to_string( resp.GetStatusCode() ) );

          if ( stm != nullptr )
          {
            char  buffer[1024];
            int   cbread;

            while ( (cbread = stm->Get( buffer, sizeof(buffer) )) > 0 )
              fwrite( buffer, cbread, 1, stdout );
          }
        }
          else
        {
          fprintf( stdout, "%s\n",
            status == http::ExecStatus::Failed ? "Failed" :
            status == http::ExecStatus::Invalid ? "Invalid" :
            status == http::ExecStatus::TimedOut ? "TimedOut" : "Undefined" );
        }
      } )
    .Start( get_channel );

  fprintf( stdout, "Done send\n" );
/*  r.wait();

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
  }*/
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
      DeliriX::ParseFB2( &text, buffer );
    }
    catch ( ... )
    {  return (void)fprintf( stderr, "unknown exception parsing fb2 from '%s'\n", archive.c_str() );  }

  // set to indexer
    StoreArchive( archive, text );

    fprintf( stdout, "Done Parse\n" );
  }
}

int   main()
{
  auto  fullfn = "/media/keva/KINGSTON/Libruks/Архивы Либрусек/4/400001.zip";

  ParseArchive( fullfn );

  fprintf( stdout, "ready to cleanup\n" );

  return 0;
}