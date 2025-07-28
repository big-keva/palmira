# if !defined( __palmira_status_report_hpp__ )
# define __palmira_status_report_hpp__
# include <mtc/zmap-alias.h>

namespace palmira {

  zmap_view( Status )
    zmap_value( int32_t,      errCode, "errCode" )
    zmap_value( mtc::charstr, message, "message" )
    zmap_value( mtc::charstr, details, "details" )

    Status( int32_t err, const char* msg, const std::initializer_list<std::pair<mtc::zmap::key, mtc::zval>>& attr = {} ):
      alias_base( { { "errCode", err }, { "message", msg } }, attr )  {}
    Status( int32_t err, const std::string& msg, const std::initializer_list<std::pair<mtc::zmap::key, mtc::zval>>& attr = {} ):
      alias_base( { { "errCode", err }, { "message", msg } }, attr )  {}
    Status( const Status& err, const std::initializer_list<std::pair<mtc::zmap::key, mtc::zval>>& attr = {} ):
      alias_base( err, attr )  {}
  zmap_end

  zmap_view( UpdateReport )
    zmap_value( Status, status, "status" )

    UpdateReport( const Status& st, const mtc::zmap& attr = {} ):
      mtc::alias_base( attr, { { "status", st } } )  {}
    UpdateReport( int err, const char* msg, const mtc::zmap& attr = {} ):
      mtc::alias_base( attr, { { "status", Status( err, msg ) } } )  {}
    UpdateReport( int err, const std::string& msg, const mtc::zmap& attr = {} ):
      mtc::alias_base( attr, { { "status", Status( err, msg ) } } )  {}
  zmap_end

  zmap_view_as( ModifyReport, UpdateReport )
    zmap_value( mtc::zmap, metadata, "metadata" )
  zmap_end

}

# endif   // !__palmira_status_report_hpp__
