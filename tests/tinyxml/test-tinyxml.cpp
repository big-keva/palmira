# include <DelphiX/textAPI/DOM-text.hpp>
# include <DelphiX/textAPI/DOM-dump.hpp>
# include "../../tinyxml/load-xml-text.hpp"
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

TestItEasy::RegisterFunc  test_contents_processor( []()
{
  TEST_CASE( "tinyxml/load" )
  {
    SECTION( "xml may be loaded as text" )
    {
      auto  text = DelphiX::textAPI::Document();
      auto  buff = LoadFile( "/home/keva/dev/palmira/texts/panov_demon.fb2" );

      palmira::tinyxml::LoadObject( &text, {
        { "empty-line", palmira::tinyxml::TagMode::remove },
        { "coverpage",  palmira::tinyxml::TagMode::remove },
        { "binary",     palmira::tinyxml::TagMode::remove },
        { "image",      palmira::tinyxml::TagMode::remove },
        { "section",    palmira::tinyxml::TagMode::ignore },
        { "FictionBook",palmira::tinyxml::TagMode::ignore },
        { "p",          palmira::tinyxml::TagMode::ignore } }, buff.data(), buff.size() );

      text.Serialize( DelphiX::textAPI::dump_as::Tags(
        DelphiX::textAPI::dump_as::MakeOutput( stdout ) ) );
    }
  }
} );
