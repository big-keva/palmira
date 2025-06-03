# include "../../../api/dynamic-contents.hxx"
# include "../../index/dynamic-entities.hxx"
# include "../../../api/storage-filesystem.hxx"
# include <mtc/test-it-easy.hpp>
# include <mtc/zmap.h>

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

TestItEasy::RegisterFunc  dynamic_ccontents( []()
  {
    TEST_CASE( "index/dynamic-contents" )
    {
      auto  contents = mtc::api<palmira::IContentsIndex>();

      SECTION( "dynamic::contents index may be created" )
      {
        REQUIRE_NOTHROW( contents = palmira::index::dynamic::contents().Create() );

        SECTION( "objects may be inserted to the contents index" )
        {
          contents->SetEntity( "aaa", KeyValues( {
            { "aaa", 1161 },
            { "bbb", 1262 },
            { "ccc", 1263 },
            { "ddd", 1264 },
            { "eee", 1265 } } ).ptr() );
        }
        SECTION( "if dynamic contents index is saved without storage specified, it throws logic_error" )
        {
          REQUIRE_EXCEPTION( contents->FlushSink(), std::logic_error );
        }
      }
      SECTION( "dynamic::contents index may be created with entity count limitation" )
      {
        REQUIRE_NOTHROW( contents = palmira::index::dynamic::contents()
          .SetMaxEntitiesCount( 3 ).Create() );

        SECTION( "insertion of more entities than the limit causes count_overflow" )
        {
          REQUIRE_NOTHROW( contents->SetEntity( "aaa", KeyValues( {
            { "aaa", 1161 } } ).ptr() ) );
          REQUIRE_NOTHROW( contents->SetEntity( "bbb", KeyValues( {
            { "bbb", 1161 } } ).ptr() ) );
          REQUIRE_EXCEPTION( contents->SetEntity( "ccc", KeyValues( {
            { "ccc", 1161 } } ).ptr() ), palmira::index::dynamic::index_overflow );
        }
      }
      SECTION( "dynamic::contents index may be created with size count limitation" )
      {
        REQUIRE_NOTHROW( contents = palmira::index::dynamic::contents()
          .SetMaxEntitiesCount( 3 )
          .SetAllocationLimit( 168480 ).Create() );

        SECTION( "insertion of more entities than the limit causes count_overflow" )
        {
          REQUIRE_NOTHROW( contents->SetEntity( "aaa", KeyValues( {
            { "aaa", 1161 } } ).ptr() ) );
          REQUIRE_EXCEPTION( contents->SetEntity( "bbb", KeyValues( {
            { "bbb", 1161 } } ).ptr() ), palmira::index::dynamic::index_overflow );
        }
      }
      SECTION( "created with storage sink, it saves index as static" )
      {
        auto  sink = palmira::storage::filesys::CreateSink( "/tmp/k2" );
        auto  well = mtc::api<palmira::IStorage::ISerialized>();

        REQUIRE_NOTHROW( contents = palmira::index::dynamic::contents()
          .SetOutStorageSink( sink )
          .Create() );

        REQUIRE_NOTHROW( contents->SetEntity( "aaa", KeyValues( { { "aaa", 1161 } } ).ptr() ) );
        REQUIRE_NOTHROW( contents->SetEntity( "bbb", KeyValues( { { "bbb", 1162 } } ).ptr() ) );
        REQUIRE_NOTHROW( contents->SetEntity( "ccc", KeyValues( { { "ccc", 1163 } } ).ptr() ) );

        if ( REQUIRE_NOTHROW( well = contents->FlushSink() ) && REQUIRE( well != nullptr ) )
          REQUIRE_NOTHROW( well->Remove() );
        contents = nullptr;
      }
    }
  } );
