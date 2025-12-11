# if !defined( __palmira_server_hpp__ )
# define __palmira_server_hpp__
# include <mtc/interfaces.h>

namespace palmira
{

  struct IServer: mtc::Iface
  {
    virtual void  Start() = 0;
    virtual void  Stop()  = 0;
    virtual void  Wait()  = 0;
  };

}

# endif // !__palmira_server_hpp__
