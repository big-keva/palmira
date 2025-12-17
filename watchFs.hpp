# if !defined( __palmira_watchFs_hpp__ )
# define __palmira_watchFs_hpp__
# include <functional>
# include <thread>
# include <string>
# include <memory>

namespace palmira {

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
    struct WatchData;

    std::shared_ptr<WatchData>  watchPtr;
    std::thread                 watchThr;
    NotifyFn                    notifyFn;

  };

}

# endif   // !__palmira_watchFs_hpp__
