#include "../errors.h"
//-------------------------------------------------------------------------//
namespace palmira::modules {
//-------------------------------------------------------------------------//
  palmira_error::palmira_error(const char *msg) : base_class(msg), ecode(-1) {
  }

  palmira_error::palmira_error(int code, const char *msg) : base_class(msg), ecode(code) {
  }
//-------------------------------------------------------------------------//
  auto check_errors(const mtc::zmap &zmap) -> void {
    const auto &status = zmap.get_zmap("status", {});
    if (not status.empty()) {
      if (status.get_int32("code", 0) != 0) {
        throw (palmira_error(status.get_int32("code", 0), status.get_charstr("info", "").c_str()));
      }
    }
  }
//-------------------------------------------------------------------------//
} // namespace palmira::modules
