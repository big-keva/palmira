# include "../servers.hpp"
# include "../plugins.hpp"

template <>
inline  std::string*  Serialize( std::string* o, const void* p, size_t l )
{
  return o->append( (const char*)p, l ), o;
}

namespace palmira::servers
{

  auto  LoadServer( mtc::api<IService> service, const char* mdpath, const mtc::config& cfgsec ) -> mtc::api<IServer>
  {
    auto  create = (FnCreateServer)LoadModule( mdpath, "CreateServer" );   // throws mtc::SharedLibrary::error
    auto  server = mtc::api<IServer>();
    int   nerror = create( server, service, cfgsec );

    if ( nerror != 0 )
      throw error( mtc::strprintf( "could not create module '%s' server, error code %d", mdpath, nerror ) );

    return server;
  }

  auto  LoadServer( mtc::api<IService> service, const mtc::config& config, const mtc::config& section ) -> mtc::api<IServer>
  {
    auto  module = section.get_path( "module" );
    auto  cfgval = section.to_zmap().get( "config" );
    auto  cfgsec = mtc::config();

  // check if module is defined
    if ( module.empty() )
      throw error( "undefined 'module' path" );

  // get configuration data
    if ( cfgval != nullptr )
    {
      switch ( cfgval->get_type() )
      {
        case mtc::zval::z_charstr:
          if ( (cfgsec = config.get_config( *cfgval->get_charstr() )).empty() )
            throw error( mtc::strprintf( "'config' points to non-ecxisting section '%s'", cfgval->get_charstr() ) );
          break;
        case mtc::zval::z_zmap:
          cfgsec = section.get_config( "config" );
          break;
        default:
          throw error( "invalid 'config' variable value type" );
      }
    }

  // create the module
    return LoadServer( service, module.c_str(), cfgsec );
  }

  auto  LoadServer( mtc::api<IService>, const mtc::config&, const mtc::charstr& ) -> mtc::api<IServer>;
  auto  LoadServer( mtc::api<IService>, const mtc::config&, const mtc::zmap& ) -> mtc::api<IServer>;
  template <class T>
  auto  GetServers( mtc::api<IService>, const mtc::config&, const std::vector<T>& ) -> std::vector<mtc::api<IServer>>;

 /*
  * GetServers( const mtc::config& config ) -> std::vector<mtc::api<palmira::IServer>>
  *
  * get/initialize servers from 'api' section:
  *
  * "api": "section_key"
  *
  * "api": {
  *   "module": ...,
  *   "config": ...
  * }
  *
  * "api": [
  *   "section_key",
  *   {
  *     "module": ...,
  *     "config": ...
  *   }
  * ]
  */
  auto  GetServers( mtc::api<IService> service, const mtc::config& config ) -> std::vector<mtc::api<IServer>>
  {
    auto  output = std::vector<mtc::api<IServer>>();
    auto  pvalue = config.to_zmap().get( "api" );

    if ( pvalue != nullptr )
    {
      switch ( pvalue->get_type() )
      {
        case mtc::zval::z_charstr:        // as section key
          output.push_back( LoadServer( service, config, *pvalue->get_charstr() ) );
          break;
        case mtc::zval::z_zmap:           // as section
          output.push_back( LoadServer( service, config, *pvalue->get_zmap() ) );
          break;
        case mtc::zval::z_array_charstr:  // as array of section keys
          output = GetServers( service, config, *pvalue->get_array_charstr() );
          break;
        case mtc::zval::z_array_zmap:     // as array of sections
          output = GetServers( service, config, *pvalue->get_array_zmap() );
          break;
        case mtc::zval::z_array_zval:     // as mixed array
          output = GetServers( service, config, *pvalue->get_array_zval() );
          break;
        default:
          throw error( "invalid 'api' server description value type" );
      }
    }
    return output;
  }

  auto  LoadServer( mtc::api<IService> service, const mtc::config& config, const mtc::charstr& key ) -> mtc::api<IServer>
  {
    return LoadServer( service, config, config.get_section( key ) );
  }

  auto  LoadServer( mtc::api<IService> service, const mtc::config& config, const mtc::zmap& sec ) -> mtc::api<IServer>
  {
    return LoadServer( service, config, mtc::config( sec, config.get_path( "" ) ) );
  }

  auto  LoadServer( mtc::api<IService> service, const mtc::config& config, const mtc::zval& next ) -> mtc::api<IServer>
  {
    if ( next.get_type() == mtc::zval::z_charstr )
      return LoadServer( service, config, *next.get_charstr() );
    if ( next.get_type() == mtc::zval::z_zmap )
      return LoadServer( service, config, *next.get_zmap() );
    throw error( "invalid 'api' server description value type" );
  }

  template <class T>
  auto  GetServers( mtc::api<IService> service, const mtc::config& config, const std::vector<T>& list ) -> std::vector<mtc::api<IServer>>
  {
    auto  output = std::vector<mtc::api<IServer>>();

    for ( auto& next: list )
      output.push_back( LoadServer( service, config, next ) );

    return output;
  }
}
