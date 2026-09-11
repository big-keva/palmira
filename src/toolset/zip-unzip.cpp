# include "zip-unzip.hpp"
# include <zlib.h>

namespace palmira
{

  auto  ZipBuf( const mtc::span<const char>& src ) -> std::vector<char>
  {
    if ( src.empty() )
      return {};

    // 1. Инициализируем структуру z_stream
    auto  stream = z_stream{};
      stream.next_in  = (Bytef*)src.data();
      stream.avail_in = src.size();

  // 15 — стандартное окно (max)
  // 8 — стандартное использование памяти
  // Z_HUFFMAN_ONLY — полностью отключает поиск совпадений строк (longest_match)
    if ( deflateInit2( &stream, Z_BEST_SPEED, Z_DEFLATED, 15, 8, Z_HUFFMAN_ONLY ) != Z_OK )
      throw std::runtime_error( "deflateInit2 failed" );

    auto  ccpack = compressBound( src.size() );
    auto  packed = std::vector<char>( ccpack );

    stream.next_out  = (Bytef*)packed.data();
    stream.avail_out = ccpack;

    // 2. Сжимаем за один проход
    int nerror = deflate( &stream, Z_FINISH );

    // Получаем реальный размер сжатых данных
    ccpack = stream.total_out;

    // Очищаем контекст zlib
    deflateEnd( &stream );

    if ( nerror != Z_STREAM_END )
      throw std::range_error( "compression failed" );

    if ( ccpack > src.size() - 100 )
      throw std::range_error( "ignore compression" );

    return packed.resize( ccpack ), packed;
  }

  auto  Unpack( const mtc::span<const char>& src ) -> std::vector<char>
  {
    if ( src.empty() )
      return {};

    // 1. Инициализируем структуру z_stream
    auto  stream = z_stream{};
      stream.next_in  = (Bytef*)src.data();
      stream.avail_in = src.size();
    auto  unpack = std::vector<char>( src.size() * 2 );

    if ( inflateInit( &stream ) != Z_OK )
      throw std::runtime_error( "inflateInit failed" );

    stream.next_out  = (Bytef*)unpack.data();
    stream.avail_out = unpack.size();


    for ( ; ; )
    {
      auto  nerror = inflate( &stream, Z_NO_FLUSH );

      // Если поток успешно закончился — выходим из цикла
      if ( nerror == Z_STREAM_END )
        break;

      // Если буфер закончился, увеличиваем его размер
      if ( nerror == Z_OK && stream.avail_out == 0 )
      {
        size_t old_size = unpack.size();
        size_t new_size = old_size * 3 / 2; // Ваша пропорция увеличения

        unpack.resize( new_size );

        // Настраиваем указатели zlib на новый свободный хвост буфера
        stream.next_out  = (Bytef*)unpack.data() + old_size;
        stream.avail_out = new_size - old_size;
      }
        else
      {
        // Произошла какая-то ошибка (например, Z_DATA_ERROR при битых данных)
        inflateEnd( &stream );
        return {}; // Возвращаем пустой вектор, как в вашей исходной логике
      }
    }

    // Получаем финальный точный размер распакованных данных
    unpack.resize( stream.total_out );
    inflateEnd( &stream );

    return unpack;
  }

}
