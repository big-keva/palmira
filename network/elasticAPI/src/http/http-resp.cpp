#include "http-resp.h"
//-------------------------------------------------------------------------//
#include <chrono>
#include <string>
#include <filesystem>
//-------------------------------------------------------------------------//
#include "network/elasticAPI/src/logger/logger.h"

#include <App.h>
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
  void send_json_response(response_context<SSL> *ctx, const service_response &response) {
    assert(ctx != nullptr && "Invalid response context");
    if (ctx->aborted.load(std::memory_order_acquire)) {
      return;
    }

    if (ctx->response == nullptr) {
      return;
    }

    ctx->response->writeStatus(http::to_string(response.status));
    ctx->response->writeHeader("Content-Type", response.content_type.empty() ? "application/json" : response.content_type);
    ctx->response->writeHeader("Content-Length", std::to_string(response.body.size()));
    ctx->response->end(response.body);
  }

  template<bool SSL>
  void send_error_response(response_context<SSL> *ctx, status_codes status, std::string_view error_type, std::string_view reason) {
    service_response response{
      .status = status,
      .content_type = "application/json",
      .body = ""
    };

    response.body.reserve(error_type.size() + reason.size() + 96);

    response.body += "{";
    response.body += R"("error":{)";
    response.body += R"("type":")";
    response.body += to_escape(error_type);
    response.body += R"(",)";
    response.body += R"("reason":")";
    response.body += to_escape(reason);
    response.body += R"(")";
    response.body += "},";
    response.body += R"("status":)";
    response.body += http::to_string(status);
    response.body += "}";

    send_json_response(ctx, response);
  }
//-------------------------------------------------------------------------//
  template
  void send_json_response<false>(response_context<false> *, const service_response &);

  template
  void send_json_response<true>(response_context<true> *, const service_response&);

  template
  void send_error_response<false>(response_context<false> *, http::status_codes, std::string_view, std::string_view);

  template
  void send_error_response<true>(response_context<true> *, http::status_codes, std::string_view, std::string_view);
//-------------------------------------------------------------------------//
}// namespace elastic::http
