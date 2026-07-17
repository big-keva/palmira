# if !defined( __palmira_toolset_hpp__ )
# define __palmira_toolset_hpp__
# include "service.hpp"

namespace palmira
{
  auto  Immediate( const mtc::zmap&, IService::NotifyFn = {} ) -> mtc::api<IService::IPending>;

 /*
  * void  AddModule( ... )
  *
  * Регистрирует функцию по имени и имени модуля
  */
  void  AddModule(
    const char* moduleName,
    const char* methodName,
    void*       methodAddr );

 /*
  * void  GetModule( ... )
  *
  * Ищет функцию по имени и имени модуля
  */
  void* GetModule(
    const char* moduleName,
    const char* methodName );

}

# endif // !__palmira_toolset_hpp__
