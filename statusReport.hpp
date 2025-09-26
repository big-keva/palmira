# if !defined( __palmira_status_report_hpp__ )
# define __palmira_status_report_hpp__
# include <mtc/zmap-alias.h>

namespace palmira {

  zmap_view( Status )
    zmap_value( int32_t,      code, "code" )
    zmap_value( mtc::charstr, info, "info" )

    Status( int32_t err, const char* msg, const std::initializer_list<std::pair<mtc::zmap::key, mtc::zval>>& attr = {} ):
      alias_base( { { "code", err }, { "info", msg } }, attr )  {}
    Status( int32_t err, const std::string& msg, const std::initializer_list<std::pair<mtc::zmap::key, mtc::zval>>& attr = {} ):
      alias_base( { { "code", err }, { "info", msg } }, attr )  {}
    Status( const Status& err, const std::initializer_list<std::pair<mtc::zmap::key, mtc::zval>>& attr = {} ):
      alias_base( err, attr )  {}
  zmap_end

  zmap_view( StatusReport )
    zmap_value( Status, status, "status" )

    StatusReport( const Status& st, const mtc::zmap& attr = {} ): alias_base( attr,
      { { "status", st } } )  {}
    StatusReport( int err, const char* msg, const mtc::zmap& attr = {} ): alias_base( attr,
      { { "status", Status( err, msg ) } } )  {}
    StatusReport( int err, const std::string& msg, const mtc::zmap& attr = {} ): alias_base( attr,
      { { "status", Status( err, msg ) } } )  {}
  zmap_end

  zmap_view_as( UpdateReport, StatusReport )
    zmap_value( mtc::zmap, metadata, "metadata" )
  zmap_end

  zmap_view_as( SearchReport, StatusReport )
    zmap_value( uint32_t, first, "first" )
    zmap_value( uint32_t, count, "count" )
  zmap_end

}

# endif   // !__palmira_status_report_hpp__
