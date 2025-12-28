# if !defined( __palmira_plugins_hpp__ )
# define __palmira_plugins_hpp__
# include <type_traits>
# include <functional>
# include <string>

namespace palmira
{

  void* LoadModule( const char* path, const char* name );

  template <class FuncPrototype, class... Args>
  auto  LoadPlugin( const char* path, const char* name, Args&&... args ) -> decltype(auto)
  {
    using CallFunc = std::add_pointer_t<std::remove_pointer_t<FuncPrototype>>;

    static_assert(std::is_function_v<std::remove_pointer_t<FuncPrototype>>,
      "CreateModule<> template parameter must be a function prototype" );
    static_assert(std::is_invocable_v<CallFunc, Args...>,
      "the specified function prototype could not be called with arguments passed" );

    return std::invoke( reinterpret_cast<CallFunc>( LoadModule( path, name ) ),
      std::forward<Args>( args )... );
  }

  template <class FuncPrototype, class... Args>
  auto  LoadPlugin( const std::string& path, const char* name, Args&&... args ) -> decltype(auto)
  {
    return LoadPlugin<FuncPrototype, Args...>( path.c_str(), name, std::forward<Args>( args )... );
  }

}

# endif // !__palmira_plugins_hpp__
