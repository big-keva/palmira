# include <toolset.hpp>
# include <server.hpp>
# include <mtc/config.h>

extern "C"  int   CreateServer( palmira::IServer**, palmira::IService*, const mtc::config& );

void  RegisterRestAPI()
{
  palmira::AddModule( "RESTAPI", "CreateServer", (void*)CreateServer );
}

namespace restAPI
{
  struct AutoInit
  {
    AutoInit()  {  RegisterRestAPI();  }
  };

  AutoInit autoInit;
}
