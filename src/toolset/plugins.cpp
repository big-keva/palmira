# include "../../plugins.hpp"
# include <mtc/sharedLibrary.hpp>
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/wcsstr.h>
# include <unistd.h>
# include <functional>
# include <mutex>
# include <map>

namespace palmira
{
  static std::map<std::string, mtc::SharedLibrary> libraries;
  static std::mutex                                liblocker;

  char* fullpath( char* out, size_t cch, const char* psz )
  {
    char    curdir[0x400];
    char    getdir[0x400];
    char    newdir[0x400];
    size_t  srclen = psz != nullptr ? mtc::w_strlen( psz ) : 0;
    char*   pslash;

    if ( psz == nullptr || srclen >= sizeof(getdir) )
      return nullptr;

    // get argument dir
    for ( pslash = mtc::w_strcpy( getdir, psz ) + srclen; pslash > getdir && *pslash != '/' && *pslash != '\\'; --pslash )
      (void)NULL;
    *pslash = '\0';

    // save current
    (void)(getcwd( curdir, sizeof(curdir) ) != nullptr );

    if ( getdir[0] != '\0' && chdir( getdir ) != 0 )
      return chdir( curdir ), nullptr;

    (void)(getcwd( newdir, sizeof(newdir) ) != nullptr );
    (void)(chdir ( curdir ) == 0 );

    for ( pslash = (char*)psz + srclen; pslash > psz && *pslash != '/' && *pslash != '\\'; --pslash )
      (void)NULL;
    while ( *pslash == '/' || *pslash == '\\' )
      ++pslash;

    if ( mtc::w_strlen( newdir ) + 2 + mtc::w_strlen( pslash ) > cch )
      return nullptr;

    return mtc::w_strcat( mtc::w_strcat( mtc::w_strcpy( out, newdir ), "/" ), pslash );
  }

  void* LoadModule( const char* path, const char* name )
  {
    char  stbuff[1024];
    auto  module = mtc::SharedLibrary();

    if ( fullpath( stbuff, sizeof(stbuff), path ) == nullptr )
      throw std::invalid_argument( "invalid library path" );

    mtc::interlocked( mtc::make_unique_lock( liblocker ), [&]() -> void
      {
        auto  loaded = libraries.find( stbuff );

        if ( loaded == libraries.end() )
          libraries.insert( { stbuff, module = mtc::SharedLibrary::Load( stbuff ) } );
        else module = loaded->second;
      } );

    return module.Find( name, mtc::enable_exceptions );
  }

}
