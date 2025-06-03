# include "../../../api/dynamic-contents.hxx"
# include "../../index/dynamic-entities.hxx"
# include "../../../api/storage-filesystem.hxx"
# include <mtc/test-it-easy.hpp>
# include <mtc/zmap.h>

#include "../../../api/static-contents.hxx"

class KeyValues: public palmira::IContents, protected mtc::zmap
{
  implement_lifetime_stub

public:
  KeyValues( const mtc::zmap& keyval ):
    zmap( keyval )  {}

  auto  ptr() const -> const palmira::IContents*
    {  return this;  }

  void  Enumerate( palmira::IContentsIndex::IKeyValue* to ) const override
    {
      for ( auto keyvalue: *this )
      {
        to->Insert( { (const char*)keyvalue.first.data(), keyvalue.first.size() },
          keyvalue.second.to_string() );
      }
    }
};

TestItEasy::RegisterFunc  static_ccontents( []()
  {
    TEST_CASE( "index/static-contents" )
    {
      SECTION( "static::contents index is created from serialized dynamic contents" )
      {
        auto  contents = mtc::api<palmira::IContentsIndex>();
        auto  serialized = mtc::api<palmira::IStorage::ISerialized>();

        REQUIRE_NOTHROW( contents = palmira::index::dynamic::contents()
          .SetOutStorageSink( palmira::storage::filesys::CreateSink( "/tmp/k2" ) ).Create() );
        REQUIRE_NOTHROW( contents->SetEntity( "aaa", KeyValues( {
            { "aaa", 1161 },
            { "bbb", 1262 },
            { "ccc", 1263 } } ).ptr() ) );
        REQUIRE_NOTHROW( contents->SetEntity( "bbb", KeyValues( {
            { "bbb", 1262 },
            { "ccc", 1263 },
            { "ddd", 1264 } } ).ptr() ) );
        REQUIRE_NOTHROW( contents->SetEntity( "ccc", KeyValues( {
            { "ccc", 1263 },
            { "ddd", 1264 },
            { "eee", 1265 } } ).ptr() ) );
        REQUIRE_NOTHROW( serialized = contents->Commit() );

        if ( REQUIRE_NOTHROW( contents = palmira::index::static_::contents().Create( serialized ) )
          && REQUIRE( contents != nullptr ) )
        {

        }
      }
    }
  } );
