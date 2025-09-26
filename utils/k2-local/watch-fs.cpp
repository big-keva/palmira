# include "watch-fs.hpp"
# include <mtc/directory.h>
# include <sys/inotify.h>
# include <functional>
# include <stdexcept>
# include <unistd.h>
# include <thread>
# include <string>

namespace palmira {
namespace k2_find {

  enum: size_t
  {
    event_size = sizeof(inotify_event),
    buffer_len = 1024 * (event_size + 16)
  };

  WatchDir::WatchDir( NotifyFn fn ):
    notifyFn( fn )  {}

  WatchDir::~WatchDir()
  {
    if ( notifyFd >= 0 )
    {
      for ( auto& nextWd: watchSet )
        inotify_rm_watch( notifyFd, nextWd.first );
      close( notifyFd );
    }
    if ( dirWatch.joinable() )
      dirWatch.join();
  }

  void  WatchDir::AddWatch( std::string dir, bool withSubdirectories )
  {
    auto  ignoreIt = std::initializer_list<const char*>{
      "/dev",
      "/run",
      "/var/run",
      "/proc",
      "/snap"
      "/sys"
    };

    int   fidWatch;
    auto  dirFetch = mtc::directory();

    if ( notifyFd == -1 && (notifyFd = inotify_init()) < 0 )
      throw std::runtime_error( "could not initialize directory monitor" );

    for ( auto& ignore: ignoreIt )
      if ( dir == ignore )
        return;

    if ( dir.length() != 0 && dir.back() != '/' )
      dir += '/';

    if ( (fidWatch = inotify_add_watch( notifyFd, dir.c_str(), IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO )) < 0 )
      return;

    watchSet.emplace( fidWatch, dir );

    if ( !dirWatch.joinable() )
      dirWatch = std::thread( &WatchDir::DirWatch, this );

    if ( withSubdirectories && (dirFetch = mtc::directory::Open( dir.c_str(), mtc::directory::attr_any )).defined() )
      for ( auto dirent = dirFetch.Get(); dirent.defined(); dirent = dirFetch.Get() )
        if ( *dirent.string() != '.' && (dirent.attrib() & mtc::directory::attr_dir) != 0 )
          AddWatch( std::string( dirent.folder() ) + dirent.string(), withSubdirectories );
  }

  void  WatchDir::DirWatch()
  {
    char  buffer[buffer_len];
    int   cbread;

    while ( (cbread = read( notifyFd, buffer, buffer_len )) >= 0 )
    {
      for ( auto beg = buffer, end = buffer + cbread; beg < end; )
      {
        auto& evNext = *reinterpret_cast<struct inotify_event*>( beg );
          beg += event_size + evNext.len;
        auto  thedir = watchSet.find( evNext.wd );

        if ( thedir == watchSet.end() )
          continue;

        if ( evNext.len == 0 )
          continue;

        // for directories created, add watch
        if ( evNext.mask & IN_ISDIR )
        {
          if ( evNext.mask & IN_CREATE )
          {
            AddWatch( thedir->second + evNext.name, true );
          }
            else
          if ( evNext.mask & IN_MODIFY )
          {
            throw std::runtime_error( "--- not supported ---" );
          }
            else
          if ( evNext.mask & IN_DELETE )
          {
            inotify_rm_watch( notifyFd, evNext.wd );
            watchSet.erase( evNext.wd );
          }
        }
          else
        if ( evNext.mask & IN_CREATE )      notifyFn( create_file, thedir->second + evNext.name );
          else
        if ( evNext.mask & IN_MODIFY )      notifyFn( modify_file, thedir->second + evNext.name );
          else
        if ( evNext.mask & IN_DELETE )      notifyFn( delete_file, thedir->second + evNext.name );
          else
        if ( evNext.mask & IN_MOVED_FROM )  notifyFn( delete_file, thedir->second + evNext.name );
          else
        if ( evNext.mask & IN_MOVED_TO )    notifyFn( create_file, thedir->second + evNext.name );
      }
    }
  }

}}
