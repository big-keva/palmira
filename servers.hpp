# if !defined( PALMIRA_SERVERS_HPP_ )
# define PALMIRA_SERVERS_HPP_
# include "../server.hpp"
# include "../service.hpp"
# include <mtc/config.h>

namespace palmira::servers
{

  typedef int (*FnCreateServer)( IServer**, IService*, const mtc::config& );

  class error: public std::runtime_error  {  using runtime_error::runtime_error;  };

 /*
  * LoadServer( ..., const mtc::config& section ) -> mtc::api<IServer>
  *
  * initialize server API from the specified section:
  * {
  *   "module": "...",
  *  ["config": "..." OR {}]
  * }
  */
  auto  LoadServer( mtc::api<IService>, const mtc::config&, const mtc::config& section ) -> mtc::api<IServer>;

 /*
  * GetServers( ..., const mtc::config& ) -> std::vector<mtc::api<IServer>>
  *
  * initialize all the servers listed in "api" key of config
  */
  auto  GetServers( mtc::api<IService>, const mtc::config& ) -> mtc::api<IServer>;

}

# endif   // !PALMIRA_SERVERS_HPP_
