# include "../../toolset.hpp"
# include <list>

namespace palmira {
namespace modules {

  struct ModuleInfo
  {
    std::string name;
    std::string meth;
    void*       addr;
  };

  auto  get_module_list() -> std::list<ModuleInfo>&
  {
    static std::list<ModuleInfo>
      moduleList;
    return moduleList;
  }

}

  void  AddModule(
    const char* moduleName,
    const char* methodName,
    void*       methodAddr )
  {
    auto& mdlist = modules::get_module_list();
    auto  modptr = std::find_if( mdlist.begin(), mdlist.end(), [&]( const modules::ModuleInfo& md )
      {  return md.name == moduleName && md.meth == methodName;  } );

    if ( modptr == mdlist.end() ) modules::get_module_list().push_back( { moduleName, methodName, methodAddr });
      else modptr->addr = methodAddr;
  }

  void*  GetModule(
    const char* moduleName,
    const char* methodName )
  {
    auto& mdlist = modules::get_module_list();
    auto  modptr = std::find_if( mdlist.begin(), mdlist.end(), [&]( const modules::ModuleInfo& md )
      {  return md.name == moduleName && md.meth == methodName;  } );

    return modptr != mdlist.end() ? modptr->addr : nullptr;
  }

}
