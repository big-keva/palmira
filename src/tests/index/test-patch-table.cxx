# include "../src/index/patch-table.hxx"
# include <mtc/test-it-easy.hpp>
# include <mtc/arena.hpp>
# include <thread>

using namespace palmira::index;

TestItEasy::RegisterFunc  patch_table( []()
  {
    TEST_CASE( "index/patch-table" )
    {
      SECTION( "patch table may be created with default allocator" )
      {
        PatchTable patch_table( 301 );

        SECTION( "patch records may be added and accessed by two type of keys:" )
        {
          SECTION( "* integer keys" )
          {
            REQUIRE_NOTHROW( patch_table.Delete( 111 ) );
            REQUIRE_NOTHROW( patch_table.Delete( 333 ) );
            REQUIRE_NOTHROW( patch_table.Delete( 777 ) );
            REQUIRE_NOTHROW( patch_table.Delete( 999 ) );

            if ( REQUIRE( patch_table.Search( 111 ) != nullptr ) )
              REQUIRE( patch_table.Search( 111 ).IsDeleted() );

            if ( REQUIRE( patch_table.Search( 333 ) != nullptr ) )
              REQUIRE( patch_table.Search( 333 ).IsDeleted() );

            if ( REQUIRE( patch_table.Search( 777 ) != nullptr ) )
              REQUIRE( patch_table.Search( 777 ).IsDeleted() );

            if ( REQUIRE( patch_table.Search( 999 ) != nullptr ) )
              REQUIRE( patch_table.Search( 999 ).IsDeleted() );

            REQUIRE( patch_table.Search( 222 ) == nullptr );
          }
          SECTION( "* string_view keys" )
          {
            auto  aaa = std::string( "aaa" );

            REQUIRE_NOTHROW( patch_table.Delete( aaa ) );

            if ( REQUIRE_NOTHROW( patch_table.Search( "aaa" ) ) )
              if ( REQUIRE( patch_table.Search( "aaa" ) != nullptr ) )
                REQUIRE( patch_table.Search( "aaa" ).IsDeleted() );

            if ( REQUIRE_NOTHROW( patch_table.Search( "bbb" ) ) )
              REQUIRE( patch_table.Search( "bbb" ) == nullptr );
          }
        }
        SECTION( "Update() patch records allocate patch data" )
        {
          if ( REQUIRE_NOTHROW( patch_table.Update( 0, { "abc" } ) ) )
            if ( REQUIRE( patch_table.Search( 0 ) != nullptr ) )
              REQUIRE( patch_table.Search( 0 ).IsUpdated() );

          SECTION( "it overrides old patch data" )
          {
            if ( REQUIRE_NOTHROW( patch_table.Update( 0, { "def" } ) ) )
              if ( REQUIRE( patch_table.Search( 0 ) != nullptr ) )
                if ( REQUIRE( patch_table.Search( 0 ).IsUpdated() ) )
                {
                  REQUIRE( patch_table.Search( 0 ).size() == 3 );
                  REQUIRE( std::string_view( patch_table.Search( 0 ).data(), 3 ) == "def" );
                }
          }
          SECTION( "and does not override Delete() patches" )
          {
            if ( REQUIRE_NOTHROW( patch_table.Update( 111, { "abc" } ) ) )
              if ( REQUIRE( patch_table.Search( 111 ) != nullptr ) )
                REQUIRE( patch_table.Search( 111 ).IsDeleted() );
          }
        }
        SECTION( "Delete() patch records override patch data" )
        {
          if ( REQUIRE_NOTHROW( patch_table.Delete( 0 ) ) )
            if ( REQUIRE( patch_table.Search( 0 ) != nullptr ) )
              REQUIRE( patch_table.Search( 0 ).IsDeleted() );
        }
      }
      SECTION( "PatchTable may be created in Arena" )
      {
        auto  arena = mtc::Arena();
        auto  patch = arena.Create<PatchTable<mtc::Arena::allocator<char>>>( 303 );

        REQUIRE_NOTHROW( patch->Delete( 0 ) );
        if ( REQUIRE_NOTHROW( patch->Search( 0 ) ) )
          if ( REQUIRE( patch->Search( 0 ) != nullptr ) )
            REQUIRE( patch->Search( 0 ).IsDeleted() );

        REQUIRE_NOTHROW( patch->Update( "aaa", "updated" ) );
        if ( REQUIRE_NOTHROW( patch->Search( "aaa" ) ) )
          if ( REQUIRE( patch->Search( "aaa" ) != nullptr ) )
          {
            REQUIRE( patch->Search( "aaa" ).IsUpdated() );
            REQUIRE( patch->Search( "aaa" ).size() == 7 );
            REQUIRE( std::string_view( patch->Search( "aaa" ).data(), 7 ) == "updated" );
          }
      }
    }
  } );

