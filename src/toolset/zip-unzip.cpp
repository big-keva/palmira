# include "zip-unzip.hpp"
# include <zlib.h>

namespace palmira
{

  auto  ZipBuf( const mtc::span<const char>& src ) -> std::vector<char>
  {
    auto  compressed_size = compressBound( src.size() );
    auto  compressed_data = std::vector<char>( compressed_size );
    auto  compress_result = compress( (Bytef*)compressed_data.data(), &compressed_size,
      (const Bytef*)src.data(), src.size() );

    if ( compress_result != Z_OK )
      throw std::range_error( "compression failed" );

    if ( compressed_size > src.size() - 100 )
      throw std::range_error( "ignore compression" );

    return compressed_data.resize( compressed_size ), compressed_data;
  }

  auto  Unpack( const mtc::span<const char>& src ) -> std::vector<char>
  {
    std::vector<char> unpack( src.size() * 2 );
    uLongf            length;
    int               nerror;

    while ( (nerror = uncompress( (Bytef*)unpack.data(), &(length = unpack.size()),
      (const Bytef*)src.data(), src.size() )) == Z_BUF_ERROR )
        unpack.resize( unpack.size() * 3 / 2 );

    if ( nerror == Z_OK ) unpack.resize( length );
      else unpack.clear();

    return unpack;
  }

}
