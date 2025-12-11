# include "unpack.hpp"
# include <zlib.h>

namespace remoapi
{

  class StreamOnVector final: public mtc::IByteStream, protected std::vector<char>
  {
    size_t  curpos = 0;

  public:
    StreamOnVector( std::vector<char>&& v ):
      std::vector<char>( std::move( v ) )  {}
    uint32_t  Get(       void*, uint32_t ) override;
    uint32_t  Put( const void*, uint32_t ) override {  throw std::logic_error( "not implemented" );  }

    implement_lifetime_control

  };

  uint32_t  StreamOnVector::Get( void* pv, uint32_t cc )
  {
    cc = std::min( cc, uint32_t(size() - curpos) );

    memcpy( pv, data() + curpos, cc );
      curpos += cc;

    return cc;
  }

  auto  Inflate( mtc::IByteStream* src ) -> mtc::api<mtc::IByteStream>
  {
    std::vector<char> source;
    std::vector<char> output;
    z_stream          stream;
    size_t            outlen = 0;

  // get source data to decompress
    if ( src != nullptr )
    {
      char  buffer[0x400];
      int   cbread;

      while ( (cbread = src->Get( buffer, 0x400 )) > 0 )
        source.insert( source.end(), buffer, buffer + cbread );
    } else throw std::invalid_argument( "logic_error: null source stream" );

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;

    if ( inflateInit( &stream ) != Z_OK )
      throw std::runtime_error( "could not inflateInit" );

    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>( source.data() ) );
    stream.avail_in = static_cast<uInt>( source.size() );

    output.resize( source.size() * 4 );

    for ( int ret = Z_OK; ret != Z_STREAM_END; )
    {
      if ( stream.avail_out == 0 )
      {
        size_t  old_size = output.size();
        output.resize(old_size * 2);  // Удваиваем размер
      }

      stream.next_out = reinterpret_cast<Bytef*>( output.data() + outlen );
      stream.avail_out = static_cast<uInt>( output.size() - outlen );

      if ( (ret = inflate( &stream, Z_NO_FLUSH )) != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR )
      {
        inflateEnd( &stream );
        throw std::runtime_error( "data decompression error" );
      }

      outlen = output.size() - stream.avail_out;
    }

    output.resize( outlen );
    inflateEnd( &stream );

    return new StreamOnVector( std::move( output ) );
  }

  // Request processors

  auto  Unpack( const http::Request& req, mtc::IByteStream* src ) -> mtc::api<mtc::IByteStream>
  {
    auto  method = req.GetHeaders().get( "Content-Encoding" );

    if ( method == "" )
      return src;

    if ( method == "deflate" )
      return Inflate( src );

    if ( method == "gzip" )
      (void)NULL;

    throw std::invalid_argument( mtc::strprintf( "Content-Encoding=%s not supported", method.c_str() ) );
  }

}
