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

TestItEasy::RegisterFunc  static_contents( []()
  {
    TEST_CASE( "index/static-contents" )
    {
      SECTION( "static::contents index is created from serialized dynamic contents" )
      {
        auto  contents = mtc::api<palmira::IContentsIndex>();
        auto  serialized = mtc::api<palmira::IStorage::ISerialized>();

        REQUIRE_NOTHROW( contents = palmira::index::dynamic::Contents()
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
        REQUIRE_NOTHROW( contents->DelEntity( "bbb" ) );
        REQUIRE_NOTHROW( serialized = contents->Commit() );

        if ( REQUIRE_NOTHROW( contents = palmira::index::Static::Create( serialized ) )
          && REQUIRE( contents != nullptr ) )
        {
          mtc::api<palmira::IContentsIndex::IEntities>  entities;

          SECTION( "key statistics is available" )
          {
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "aaa", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "aaa", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "bbb", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "bbb", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "ccc", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "ccc", 3 ).nCount == 2 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "ddd", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "ddd", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "eee", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "eee", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "fff", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "fff", 3 ).nCount == 0 );
          }
          SECTION( "key blocks are available" )
          {
            palmira::IContentsIndex::IEntities::Reference entRef;

            if ( REQUIRE_NOTHROW( entities = contents->GetKeyBlock( "aaa", 3 ) ) && REQUIRE( entities != nullptr ) )
            {
              if ( REQUIRE_NOTHROW( entRef = entities->Find( 1 ) ) )
              {
                REQUIRE( entRef.uEntity == 1 );
                if ( REQUIRE_NOTHROW( entRef = entities->Find( entRef.uEntity + 1 ) ) )
                  REQUIRE( entRef.uEntity == uint32_t(-1) );
              }
            }

            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "bbb", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "bbb", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "ccc", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "ccc", 3 ).nCount == 2 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "ddd", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "ddd", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "eee", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "eee", 3 ).nCount == 1 );
            if ( REQUIRE_NOTHROW( contents->GetKeyStats( "fff", 3 ) ) )
              REQUIRE( contents->GetKeyStats( "fff", 3 ).nCount == 0 );
          }
          SECTION( "entity may be deleted" )
          {
            bool  deleted;

            if ( REQUIRE_NOTHROW( deleted = contents->DelEntity( "aaa" ) ) )
              REQUIRE( deleted == true );

            SECTION( "after deletion entity may not be found" )
            {
              SECTION( "- by id" )
              {
                if ( REQUIRE_NOTHROW( contents->GetEntity( "aaa" ) ) )
                  REQUIRE( contents->GetEntity( "aaa" ) == nullptr );
              }
              SECTION( "- by index" )
              {
                if ( REQUIRE_NOTHROW( contents->GetEntity( 1 ) ) )
                  REQUIRE( contents->GetEntity( 1 ) == nullptr );
              }
              SECTION( "- by contents" )
              {
                palmira::IContentsIndex::IEntities::Reference entRef;

                REQUIRE_NOTHROW( entities = contents->GetKeyBlock( "aaa", 3 ) );
                REQUIRE( entities != nullptr );

                if ( REQUIRE_NOTHROW( entRef = entities->Find( 1 ) ) )
                  REQUIRE( entRef.uEntity == uint32_t(-1) );
              }
            }
            SECTION( "second deletion of deleted object returns false" )
            {
              REQUIRE( contents->DelEntity( "aaa" ) == false );
            }
          }
        }
      }
    }
  } );
