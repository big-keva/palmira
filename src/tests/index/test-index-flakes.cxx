# include "../../index/index-flakes.hxx"
# include "../../../api/storage-filesystem.hxx"
# include "../../../api/dynamic-contents.hxx"
# include "../../../api/static-contents.hxx"
# include <mtc/test-it-easy.hpp>
# include <mtc/zmap.h>

using namespace palmira;

class KeyValues: public IContents, protected mtc::zmap
{
  implement_lifetime_stub

public:
  KeyValues( const mtc::zmap& keyval ):
    zmap( keyval )  {}

  auto  ptr() const -> const IContents*
  {  return this;  }

  void  Enumerate( IContentsIndex::IIndexAPI* to ) const override
  {
    for ( auto keyvalue: *this )
    {
      auto  val = keyvalue.second.to_string();

      to->Insert( { (const char*)keyvalue.first.data(), keyvalue.first.size() },
        { val.data(), val.size() } );
    }
  }
};

auto  CreateDynamicIndex( const mtc::zmap& ix ) -> mtc::api<IContentsIndex>
{
  auto  pstore = palmira::storage::filesys::CreateSink( "/tmp/k2" );
  auto  pindex = palmira::index::dynamic::Contents()
    .SetAllocationLimit( 2 * 1024 * 1024 )
    .SetMaxEntitiesCount( 1024 )
    .SetOutStorageSink( pstore )
    .Create();

  for ( auto& next: ix )
    pindex->SetEntity( next.first.to_charstr(), KeyValues( *next.second.get_zmap() ).ptr(), nullptr );

  return pindex;
}

auto  CreateStaticIndex( const mtc::zmap& ix ) -> mtc::api<IContentsIndex>
{
  auto  serial = CreateDynamicIndex( ix )->Commit();

  return index::Static::Create( serial );
}

TestItEasy::RegisterFunc  index_flakes( []()
  {
    TEST_CASE( "index/index-flakes" )
    {
      SECTION( "IndexFlakes manages a list of indices with index ranges" )
      {
        index::IndexFlakes flakes;

        SECTION( "uninitialized, it has no entities" )
        {
          REQUIRE( flakes.getMaxIndex() == 0 );
          REQUIRE( flakes.delEntity( "aaa" ) == false );
          REQUIRE( flakes.getEntity( "aaa" ) == nullptr );
          REQUIRE( flakes.getEntity( 1U ) == nullptr );
          REQUIRE( flakes.getKeyBlock( "aaa", 3 ) == nullptr );
          REQUIRE( flakes.getKeyStats( "aaa", 3 ).nCount == 0 );
        }
        SECTION( "begin initialized, it lists objects" )
        {
          REQUIRE_NOTHROW( flakes.add( CreateStaticIndex( {
            { "i1", mtc::zmap{
              { "aaa", "aaa" },
              { "bbb", "bbb" },
              { "ccc", "ccc" } } },
            { "i2", mtc::zmap{
              { "bbb", "bbb" },
              { "ccc", "ccc" },
              { "ddd", "ddd" } } },
            { "i3", mtc::zmap{
              { "ccc", "ccc" },
              { "ddd", "ddd" },
              { "eee", "eee" } } },
          } ) ) );
          REQUIRE_NOTHROW( flakes.add( CreateStaticIndex( {
            { "i4", mtc::zmap{
              { "ddd", "ddd" },
              { "eee", "eee" },
              { "fff", "fff" } } },
            { "i5", mtc::zmap{
              { "eee", "eee" },
              { "fff", "fff" },
              { "ggg", "ggg" } } },
            { "i6", mtc::zmap{
              { "fff", "fff" },
              { "ggg", "ggg" },
              { "hhh", "hhh" } } },
            } ) ) );
          SECTION( "entities of nested indices may be get" )
          {
            SECTION( "non-existing entities are not found" )
            {
              if ( REQUIRE_NOTHROW( flakes.getEntity( "i0" ) ) )
                REQUIRE( flakes.getEntity( "i0" ) == nullptr );
            }
            SECTION( "existing entities are found with overriden index" )
            {
              if ( REQUIRE_NOTHROW( flakes.getEntity( "i1" ) ) )
              {
                REQUIRE( flakes.getEntity( "i1" ) != nullptr );
                REQUIRE( flakes.getEntity( "i1" )->GetIndex() == 1 );
              }
              if ( REQUIRE_NOTHROW( flakes.getEntity( "i4" ) ) )
              {
                REQUIRE( flakes.getEntity( "i4" ) != nullptr );
                REQUIRE( flakes.getEntity( "i4" )->GetIndex() == 4 );
              }
            }
            SECTION( "entities may be found by contents blocks" )
            {
              auto  entities = mtc::api<IContentsIndex::IEntities>();
              auto  entref = IContentsIndex::IEntities::Reference{};

              SECTION( "contents blocks may be loaded" )
              {
                if ( REQUIRE_NOTHROW( entities = flakes.getKeyBlock( "ddd", 3 ) ) )
                {
                  SECTION( "entities references may be found sequentally" )
                  {
                    REQUIRE( (entref = entities->Find( 1 )).uEntity == 2 );
                    REQUIRE( (entref = entities->Find( 3 )).uEntity == 3 );
                    REQUIRE( (entref = entities->Find( 4 )).uEntity == 4 );
                    REQUIRE( (entref = entities->Find( 5 )).uEntity == uint32_t(-1) );
                  }
                }
              }
            }
          }
          SECTION( "entity may be deleted" )
          {
            bool  deleted;

            if ( REQUIRE_NOTHROW( deleted = flakes.delEntity( "i2" ) ) )
              REQUIRE( deleted );

            SECTION( "after deletion entity may not be get" )
            {
              SECTION( "- by id")
              {
                if ( REQUIRE_NOTHROW( flakes.getEntity( "i2" ) ) )
                  REQUIRE( flakes.getEntity( "i2" ) == nullptr );
              }
              SECTION( "- by index" )
              {
                if ( REQUIRE_NOTHROW( flakes.getEntity( 2 ) ) )
                  REQUIRE( flakes.getEntity( 2 ) == nullptr );
              }
              SECTION( "- by content references" )
              {
                auto  entities = mtc::api<IContentsIndex::IEntities>();
                auto  entref = IContentsIndex::IEntities::Reference{};

                if ( REQUIRE_NOTHROW( entities = flakes.getKeyBlock( "ddd", 3 ) ) )
                  if ( REQUIRE( entities != nullptr ) )
                  {
                    REQUIRE( (entref = entities->Find( 1 )).uEntity == 3 );
                    REQUIRE( (entref = entities->Find( 4 )).uEntity == 4 );
                    REQUIRE( (entref = entities->Find( 5 )).uEntity == uint32_t(-1) );
                  }
              }
            }
            SECTION( "second deletion of entity returns false" )
            {
              if ( REQUIRE_NOTHROW( deleted = flakes.delEntity( "i2" ) ) )
                REQUIRE( deleted == false );
            }
          }
        }
      }
    }
  } );
