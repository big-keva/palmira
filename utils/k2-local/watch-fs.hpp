# if !defined( __palmira_k2_local_watch_fs_hpp__ )
# define __palmira_k2_local_watch_fs_hpp__
# include <functional>
# include <thread>
# include <string>

namespace palmira {
namespace k2_find {

  class WatchDir
  {
    void  DirWatch();

  public:
    enum: unsigned
    {
      create_file = 1,
      modify_file = 2,
      delete_file = 3
    };

    using  NotifyFn = std::function<void(unsigned, const std::string&)>;

    WatchDir( NotifyFn );
   ~WatchDir();

    void  AddWatch( std::string, bool withSubdirectories = true );

  protected:
    int                                   notifyFd = -1;         // directory event handler
    NotifyFn                              notifyFn;
    std::unordered_map<int, std::string>  watchSet;
    std::thread                           dirWatch;

  };

}}

# endif   // !__palmira_k2_local_watch_fs_hpp__
