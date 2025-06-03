# include "../../index/dynamic-chains.hxx"
# include <mtc/test-it-easy.hpp>
# include <mtc/arena.hpp>
# include <thread>

TestItEasy::RegisterFunc  dynamic_chains( []()
  {
    TEST_CASE( "index/dynamic-chains" )
    {
      SECTION( "BlockChains may be allocated with default allocator" )
      {
        palmira::index::dynamic::BlockChains<> chains;

        SECTION( "keys may be added to BlockChains" )
        {
          REQUIRE_NOTHROW( chains.Insert( "k1", 1, {} ) );
          REQUIRE_NOTHROW( chains.Insert( "k1", 2, "xxx" ) );
          REQUIRE_NOTHROW( chains.Insert( "k2", 1, "yyy" ) );
          REQUIRE_NOTHROW( chains.Insert( "k1", 4, "zzz" ) );
          REQUIRE_NOTHROW( chains.Insert( "k1", 3, "ttt" ) );
        }
        SECTION( "non-existing keys are not found" )
        {
          REQUIRE( chains.Lookup( "k0" ) == nullptr );
        }
        SECTION( "existing keys are found" )
        {
          if ( REQUIRE( chains.Lookup( "k1" ) != nullptr ) )
            REQUIRE( chains.Lookup( "k1" )->ncount == 4 );

          if ( REQUIRE( chains.Lookup( "k2" ) != nullptr ) )
            REQUIRE( chains.Lookup( "k2" )->ncount == 1 );
        }
      }
      SECTION( "BlockChains may be created in Arena" )
      {
        auto  mArena = mtc::Arena();
        auto  chains = mArena.Create<palmira::index::dynamic::BlockChains<mtc::Arena::allocator<char>>>();

        SECTION( "arena-allocated object also is functional" )
        {
          REQUIRE_NOTHROW( chains->Insert( "k1", 1, {} ) );
          REQUIRE_NOTHROW( chains->Insert( "k1", 2, "xxx" ) );
          REQUIRE_NOTHROW( chains->Insert( "k2", 1, "yyy" ) );
          REQUIRE_NOTHROW( chains->Insert( "k1", 4, "zzz" ) );
          REQUIRE_NOTHROW( chains->Insert( "k1", 3, "ttt" ) );

          REQUIRE( chains->Lookup( "k0" ) == nullptr );

          REQUIRE( chains->Lookup( "k1" ) != nullptr );
          REQUIRE( chains->Lookup( "k2" ) != nullptr );
        }

        chains->StopIt();
      }
      SECTION( "BlockChains provide correct inserion order in multithreaded environments" )
      {
        auto  chains = palmira::index::dynamic::BlockChains<>();
        auto  thlist = std::vector<std::thread>();
        auto  runner = [&]( int startEntity, int finalEntity )
          {
            for ( auto entity = startEntity; entity <= finalEntity; ++entity )
            {
              const int nwords = 3 + int32_t(rand() * 60.0 / RAND_MAX);
              uint32_t  keyset[2] = { 0, 0 };

              for ( auto word = 0; word != nwords; )
              {
                auto  ikey = uint32_t(rand() * 63.0 / RAND_MAX);

                if ( (keyset[ikey / 32] & (1 << (ikey % 32))) == 0 )
                {
                  auto  skey = mtc::strprintf( "%u", ikey );

                  chains.Insert( skey, entity, {} );

                  keyset[ikey / 32] |= (1 << (ikey % 32));
                  ++word;
                }
              }
            }
          };

        for ( int i = 0; i < 10; ++i )
          thlist.emplace_back( std::thread( runner, 1 + i * 100, 100 + i * 100 ) );

        for ( auto& next : thlist )
          if ( next.joinable() )
            next.join();

        REQUIRE( chains.Verify() );
      }
    }
  } );
