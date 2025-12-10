# if !defined( __palmira_server_hpp__ )
# define __palmira_server_hpp__
# include <mtc/interfaces.h>

namespace palmira
{

  struct IServer: public mtc::Iface
  {
    virtual void  Start();
    virtual void  Stop();
    virtual void  Wait();
  };

}

# endif // !__palmira_server_hpp__
