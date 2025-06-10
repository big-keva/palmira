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

  void  Enumerate( palmira::IContentsIndex::IIndexAPI* to ) const override
    {
      for ( auto keyvalue: *this )
      {
        auto  val = keyvalue.second.to_string();

        to->Insert( { (const char*)keyvalue.first.data(), keyvalue.first.size() },
          { val.data(), val.size() } );
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
            { "k1", 1161 },
            { "k2", 1262 },
            { "k3", 1263 } } ).ptr() );
          contents->SetEntity( "bbb", KeyValues( {
            { "k2", 1262 },
            { "k3", 1263 },
            { "k4", 1264 } } ).ptr() );
          contents->SetEntity( "ccc", KeyValues( {
            { "k3", 1263 },
            { "k4", 1264 },
            { "k5", 1265 } } ).ptr() );
          contents->DelEntity( "bbb" );
        }
        SECTION( "objects are indexed by keys, so keys may be get" )
        {
          mtc::api<palmira::IContentsIndex::IEntities>  entities;

          if ( REQUIRE_NOTHROW( contents->GetKeyStats( "k0", 2 ) ) )
            REQUIRE( contents->GetKeyStats( "k0", 2 ).nCount == 0 );
          if ( REQUIRE_NOTHROW( contents->GetKeyStats( "k1", 2 ) ) )
          {
            REQUIRE( contents->GetKeyStats( "k1", 2 ).nCount == 1 );
            REQUIRE( contents->GetKeyStats( "k2", 2 ).nCount == 2 );
            REQUIRE( contents->GetKeyStats( "k3", 2 ).nCount == 3 );
            REQUIRE( contents->GetKeyStats( "k4", 2 ).nCount == 2 );
            REQUIRE( contents->GetKeyStats( "k5", 2 ).nCount == 1 );
          }

          if ( REQUIRE_NOTHROW( entities = contents->GetKeyBlock( "k3", 2 ) ) )
            if ( REQUIRE( entities != nullptr ) )
            {
              REQUIRE( entities->Find( 0 ).uEntity == 1U );
              REQUIRE( entities->Find( 1 ).uEntity == 1U );
              REQUIRE( entities->Find( 2 ).uEntity == 3U );
              REQUIRE( entities->Find( 3 ).uEntity == 3U );
              REQUIRE( entities->Find( 4 ).uEntity == uint32_t(-1) );
            }
        }
        SECTION( "if dynamic contents index is saved without storage specified, it throws logic_error" )
        {
          REQUIRE_EXCEPTION( contents->Commit(), std::logic_error );
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

        if ( REQUIRE_NOTHROW( well = contents->Commit() ) && REQUIRE( well != nullptr ) )
          REQUIRE_NOTHROW( well->Remove() );
        contents = nullptr;
      }
    }
  } );
