# include "../../readers/read-zip.hpp"
# include <mtc/test-it-easy.hpp>

auto  LoadFile( const char* path ) -> std::vector<char>
{
  std::vector<char> load;
  char              buff[1024];
  auto              file = fopen( path, "rb" );

  for ( auto read = fread( buff, 1, sizeof(buff), file ); read > 0; read = fread( buff, 1, sizeof(buff), file ) )
    load.insert( load.end(), buff, buff + read );

  fclose( file );

  return load;
}

TestItEasy::RegisterFunc  test_zipfile( []()
{
  TEST_CASE( "zipfile/reader" )
  {
    SECTION( "zip may be opened, listed and read" )
    {
      auto  buf = LoadFile( "/home/keva/Downloads/Libruks/Архивы Либрусек/fb2-793780-793916.zip" );

      palmira::minizip::Read( []( const mtc::span<const char>&, const std::vector<std::string>& names )
        {
          for ( size_t i = 0; i != names.size(); ++i )
            fprintf( stdout, i != 0 ? "->%s" : "%s", names[i].c_str() );
          fputc( '\n', stdout );
        }, buf );
    }
  }
} );
