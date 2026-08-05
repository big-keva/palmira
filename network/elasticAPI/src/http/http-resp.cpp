#include "http-resp.h"
//-------------------------------------------------------------------------//
#include <chrono>
#include <string>
#include <filesystem>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include <mtc/json.h>
//-------------------------------------------------------------------------//
#include "../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../common/utils.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace {
//-------------------------------------------------------------------------//
    auto to_escape(std::string_view value) -> std::string
    {
      std::string result;
      result.reserve(value.size() + 8);

      for (char c : value) {
        switch (c) {
        case '\\': result += "\\\\"; break;
        case '"':  result += "\\\""; break;
        case '\n': result += "\\n";  break;
        case '\r': result += "\\r";  break;
        case '\t': result += "\\t";  break;
        default:   result += c;      break;
        }
      }

      return result;
    }

    auto get_elapsed_ms(std::uint64_t started) -> std::uint64_t
    {
      if (started > 0) {
        auto diff_ticks = static_cast<double>(std::chrono::steady_clock::now().time_since_epoch().count() - started);
        auto nanoseconds_per_tick = static_cast<double>(std::chrono::steady_clock::duration::period::num) / std::chrono::steady_clock::duration::period::den * 1'000'000'000;

        return static_cast<std::uint64_t>((diff_ticks * nanoseconds_per_tick) / 1'000'000.0);
      }

      return 0;
    }
//-------------------------------------------------------------------------//
  }// namespace
//-------------------------------------------------------------------------//
  /*
   * Эта функция ОБЯЗАНА вызываться только из родного uWebSockets loop.
   * Нельзя вызывать её напрямую из ThreadPool.
   */
  template<bool SSL>
  void send_json_response(uWS::HttpResponse<SSL> *resp, const service_response &response) {
    assert(resp != nullptr && "Invalid response context");
    if (resp == nullptr) {
      return;
    }

    resp->writeStatus(http::to_string(response.status));
    resp->writeHeader("Content-Type", response.content_type.empty() ? "application/json" : response.content_type);
    resp->writeHeader("Content-Length", std::to_string(response.body.size()));
    resp->end(response.body);
  }

  template<bool SSL>
  void send_error_response(uWS::HttpResponse<SSL> *resp, status_codes status, std::string_view error_type, std::string_view reason) {
    service_response response{
      .status = status,
      .content_type = "application/json",
      .body = ""
    };

    response.body.reserve(error_type.size() + reason.size() + 96);

    response.body += '{';
    response.body += R"("error":{)";
    response.body += R"("type":")";
    response.body += to_escape(error_type);
    response.body += R"(",)";
    response.body += R"("reason":")";
    response.body += to_escape(reason);
    response.body += '\"';
    response.body += "},";
    response.body += R"("status":)";
    response.body += http::to_string(status);
    response.body += '}';

    send_json_response(resp, response);
  }
//-------------------------------------------------------------------------//
  template<typename response_t>
  auto send_default_response(response_t *resp) -> response_t *
  {
    static constexpr std::string_view s_body = R"json(
      {
        "name": "palmira-node-1",
        "cluster_name": "palmira",
        "cluster_uuid": "palmira-local",
        "version": {
          "number": "7.10.2",
          "build_flavor": "default",
          "build_type": "custom",
          "build_hash": "unknown",
          "build_date": "2026-08-05T00:00:00Z",
          "build_snapshot": false,
          "lucene_version": "8.7.0",
          "minimum_wire_compatibility_version": "6.8.0",
          "minimum_index_compatibility_version": "6.0.0-beta1"
        },
        "tagline": "You Know, for Search"
      })json";

    // Sending a reply.
    resp->writeHeader("Access-Control-Allow-Origin", "*")
        ->writeHeader("Access-Control-Allow-Methods", "OPTIONS, HEAD, GET, POST, PUT, DELETE")
        ->writeHeader("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Content-Length, Authorization")
        ->writeHeader("Access-Control-Max-Age", "86400")
        ->end(s_body);

    return resp;
  }
//-------------------------------------------------------------------------//
  template
  void send_json_response<false>(uWS::HttpResponse<false> *, const service_response &);

  template
  void send_json_response<true>(uWS::HttpResponse<true> *, const service_response &);

  template
  void send_error_response<false>(uWS::HttpResponse<false> *, http::status_codes, std::string_view, std::string_view);

  template
  void send_error_response<true>(uWS::HttpResponse<true> *, http::status_codes, std::string_view, std::string_view);

  template
  auto send_default_response(uWS::HttpResponse<true> *resp) -> uWS::HttpResponse<true>  *;

  template
  auto send_default_response(uWS::HttpResponse<false> *resp) -> uWS::HttpResponse<false>  *;
//-------------------------------------------------------------------------//
}// namespace elastic::http
