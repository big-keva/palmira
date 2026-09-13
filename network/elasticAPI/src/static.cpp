# include <toolset.hpp>
# include <server.hpp>
# include <mtc/config.h>

extern "C"  int   CreateServer( palmira::IServer**, palmira::IService*, const mtc::config& );

namespace
{
  void  RegisterElastic()
  {
    palmira::AddModule( "ELASTIC", "CreateServer", (void*)CreateServer );
  }
}

namespace elastic
{
  struct AutoInit
  {
    AutoInit()  {  RegisterElastic();  }
  };

  AutoInit autoInit;
}
