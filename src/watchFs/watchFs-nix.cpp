# include "../../watchFs.hpp"
# include <mtc/directory.h>
# include <mtc/recursive_shared_mutex.hpp>
# include <sys/inotify.h>
# include <functional>
# include <stdexcept>
# include <unistd.h>
# include <thread>
# include <mutex>
# include <string>

namespace palmira {

  enum: size_t
  {
    event_size = sizeof(inotify_event),
    buffer_len = 1024 * (event_size + 16)
  };

  struct WatchDir::WatchData
  {
    int                                   notifyFd = -1;         // directory event handler
    std::unordered_map<int, std::string>  watchSet;
    std::mutex                            watchMtx;

    WatchData();
   ~WatchData();
  };

  // WatchDir::WatchData implementation

  WatchDir::WatchData::WatchData()
  {
  }

  WatchDir::WatchData::~WatchData()
  {
    if ( notifyFd >= 0 )
    {
      for ( auto& nextWd: watchSet )
        inotify_rm_watch( notifyFd, nextWd.first );
      close( notifyFd );
    }
  }

  // WatchDir implementation

  WatchDir::WatchDir( NotifyFn fn ):
    watchPtr( std::make_shared<WatchData>() ),
    notifyFn( std::move( fn ) )
  {
  }

  WatchDir::~WatchDir()
  {
    watchPtr = nullptr;

    if ( watchThr.joinable() )
      watchThr.join();
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

    if ( watchPtr->notifyFd == -1 && (watchPtr->notifyFd = inotify_init()) < 0 )
      throw std::runtime_error( "could not initialize directory monitor" );

    for ( auto& ignore: ignoreIt )
      if ( dir == ignore )
        return;

    if ( dir.length() != 0 && dir.back() != '/' )
      dir += '/';

    if ( (fidWatch = inotify_add_watch( watchPtr->notifyFd, dir.c_str(), IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO )) < 0 )
      return;

    mtc::interlocked( mtc::make_unique_lock( watchPtr->watchMtx ), [&]()
      {
        watchPtr->watchSet.emplace( fidWatch, dir );
      } );

    if ( !watchThr.joinable() )
      watchThr = std::thread( &WatchDir::DirWatch, this );

    if ( withSubdirectories && (dirFetch = mtc::directory::Open( dir.c_str(), mtc::directory::attr_any )).defined() )
      for ( auto dirent = dirFetch.Get(); dirent.defined(); dirent = dirFetch.Get() )
        if ( *dirent.string() != '.' && (dirent.attrib() & mtc::directory::attr_dir) != 0 )
          AddWatch( std::string( dirent.folder() ) + dirent.string(), withSubdirectories );
  }

  void  WatchDir::DirWatch()
  {
    char  buffer[buffer_len];
    int   cbread;

    while ( (cbread = read( watchPtr->notifyFd, buffer, buffer_len )) >= 0 )
    {
      for ( auto beg = buffer, end = buffer + cbread; beg < end; )
      {
        auto& evNext = *reinterpret_cast<struct inotify_event*>( beg );
          beg += event_size + evNext.len;
        auto  exlock = mtc::make_unique_lock( watchPtr->watchMtx );
        auto  thedir = watchPtr->watchSet.find( evNext.wd );
        auto  dirstr = std::string();

        if ( evNext.len == 0 )
          continue;

        if ( thedir != watchPtr->watchSet.end() ) dirstr = thedir->second;
          else continue;

        exlock.unlock();

        // for directories created, add watch
        if ( evNext.mask & IN_ISDIR )
        {
          if ( evNext.mask & IN_CREATE )
          {
            AddWatch( dirstr + evNext.name, true );
          }
            else
          if ( evNext.mask & IN_MODIFY )
          {
            throw std::runtime_error( "--- not supported ---" );
          }
            else
          if ( evNext.mask & IN_DELETE )
          {
            inotify_rm_watch(
              watchPtr->notifyFd, evNext.wd );
            exlock.lock();
              watchPtr->watchSet.erase( evNext.wd );
            exlock.unlock();
          }
        }
          else
        if ( evNext.mask & IN_CREATE )      notifyFn( create_file, dirstr + evNext.name );
          else
        if ( evNext.mask & IN_MODIFY )      notifyFn( modify_file, dirstr + evNext.name );
          else
        if ( evNext.mask & IN_DELETE )      notifyFn( delete_file, dirstr + evNext.name );
          else
        if ( evNext.mask & IN_MOVED_FROM )  notifyFn( delete_file, dirstr + evNext.name );
          else
        if ( evNext.mask & IN_MOVED_TO )    notifyFn( create_file, dirstr + evNext.name );
      }
    }
  }

}
