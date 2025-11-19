# include "../watch-fs.hpp"
# include "tmppath.h"
# include <mtc/test-it-easy.hpp>
# include <cstdio>
#include <mutex>

using namespace palmira;

TestItEasy::RegisterFunc  test_watch_fs( []()
{
  std::string changedStr;
  std::string waitForStr;
  unsigned    changeType = unsigned(-1);

  TEST_CASE( "watch-fs" )
  {
    std::mutex                    waitMx;
    std::unique_lock<std::mutex>  locker( waitMx );
    std::condition_variable       cvWait;

    k2_find::WatchDir watchDir( [&]( unsigned u, const std::string& s )
      {
        if ( s.length() >= waitForStr.length() && s.substr( s.length() - waitForStr.length() ) == waitForStr )
        {
          const char* eventStr[4] = { "unknown", "Create", "Modify", "Delete" };

          changedStr = s;
          changeType = u;
          fprintf( stdout, "%s: %s\n", eventStr[u], changedStr.c_str() );
          fflush( stdout );
          cvWait.notify_one();
        }
      } );

    SECTION( "monitored directory may be added" )
    {
      auto  tmpdir = GetTmpPath();

      remove( (tmpdir + "new-file.txt").c_str() );

      if ( REQUIRE_NOTHROW( watchDir.AddWatch( tmpdir, true ) ) )
      {
        SECTION( "file creation causes WatchDir::create_file" )
        {
          waitForStr = "new-file.txt";

          fclose( fopen( (tmpdir + "new-file.txt").c_str(), "wb" ) );

          if ( REQUIRE( cvWait.wait_for( locker, std::chrono::seconds( 10 ), [&](){  return !changedStr.empty();  } ) ) )
            REQUIRE( changeType == k2_find::WatchDir::create_file );
        }
        SECTION( "file modification causes WatchDir::modify_file" )
        {
          changedStr.clear();
          changeType = unsigned(-1);

          auto  file = fopen( (tmpdir + "new-file.txt").c_str(), "wb" );
            fprintf( file, "this is the test file string @%llu", time( nullptr ) );
          fclose( file );

          fprintf( stdout, "modified file\n" );
          fflush( stdout );

          if ( REQUIRE( cvWait.wait_for( locker, std::chrono::seconds( 10 ), [&](){  return !changedStr.empty();  } ) ) )
            REQUIRE( changeType == k2_find::WatchDir::modify_file );
        }
      }
      char s[10];
      fgets( s, sizeof(s), stdin );
    }
  }
} );