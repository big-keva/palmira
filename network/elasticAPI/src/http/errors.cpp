#include "errors.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  auto to_string(status_codes code) -> std::string_view
  {
    switch (code)
    {
      case status_codes::OK: return "200 OK";
      case status_codes::CREATED: return "201 Created";
      case status_codes::ACCEPTED: return "202 Accepted";
      case status_codes::NO_CONTENT: return "204 No Content";

      case status_codes::BAD_REQUEST: return "400 Bad Request";
      case status_codes::UNAUTHORIZED: return "401 Unauthorized";
      case status_codes::FORBIDDEN: return "403 Forbidden";
      case status_codes::NOT_FOUND: return "404 Not Found";
      case status_codes::METHOD_NOT_ALLOWED: return "405 Method Not Allowed";
      case status_codes::CONFLICT: return "409 Conflict";
      case status_codes::PAYLOAD_TOO_LARGE: return "413 Payload Too Large";
      case status_codes::UNSUPPORTED_MEDIA_TYPE: return "415 Unsupported Media Type";
      case status_codes::TOO_MANY_REQUESTS: return "429 Too Many Requests";

      case status_codes::INTERNAL_SERVER_ERROR: return "500 Internal Server Error";
      case status_codes::NOT_IMPLEMENTED: return "501 Not Implemented";
      case status_codes::SERVICE_UNAVAILABLE: return "503 Service Unavailable";
    }
    return "unknown";
  }

  auto make_error_json(status_codes code, std::string type, std::string reason) -> std::string
  {
    return R"({"error":{"type":")" + type + R"(","reason":")" + reason + R"(","status": ")" + std::string(to_string(code)) + """}";
  }
//-------------------------------------------------------------------------//
  auto check_resp_on_error(const mtc::zmap &resp) -> error_info
  {
    error_info info{
      .code = 0,
      .msg = "success"
    };
    const auto &status = resp.get_zmap("status", {});
    info.code = not status.empty() ? status.get_int32("code", -1) : 0;
    if (info.code != 0)
    {
      if (info.code == -1)
      {
        info.code = -EINVAL;
        info.msg = "invalid format: status/code node found";
      }
      info.msg = status.get_charstr("info", "unknown");
    }
    return info;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
