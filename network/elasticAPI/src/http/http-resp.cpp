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
    template<typename Output>
    auto serialize(Output *output, const mtc::zmap &zmap, const char *braces = "{}") -> Output *;

    template<typename Output>
    auto serialize(Output *output, const mtc::array_zval &zvalues, const char *braces = "[]") -> Output *;
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

    auto get_error_context_length(std::string_view index, std::string_view docid) -> size_t
    {
      static const auto s_length = std::strlen(R"({"_index": ")") +
                                              std::strlen(R"(","_id": ")") +
                                              std::strlen(R"(","found": false})");

      return s_length + index.length() + docid.length();
    }
//-------------------------------------------------------------------------//
    template<typename Output>
    auto serialize(Output *output, const mtc::zval &zval) -> Output *
    {
      switch (zval.get_type())
      {
      case mtc::zval::z_char:    output->write(std::to_string(*zval.get_char())); break;
      case mtc::zval::z_byte:    output->write(std::to_string(*zval.get_byte())); break;
      case mtc::zval::z_int16:   output->write(std::to_string(*zval.get_int16())); break;
      case mtc::zval::z_word16:  output->write(std::to_string(*zval.get_word16())); break;
      case mtc::zval::z_int32:   output->write(std::to_string(*zval.get_int32())); break;
      case mtc::zval::z_word32:  output->write(std::to_string(*zval.get_word32())); break;
      case mtc::zval::z_int64:   output->write(std::to_string(*zval.get_int64())); break;
      case mtc::zval::z_word64:  output->write(std::to_string(*zval.get_word64())); break;
      case mtc::zval::z_float:   output->write(std::to_string(*zval.get_float())); break;
      case mtc::zval::z_double:  output->write(std::to_string(*zval.get_double())); break;
      case mtc::zval::z_bool:    output->write(*zval.get_bool() ? "true" : "false"); break;
      case mtc::zval::z_uuid:    output->write(mtc::to_string(*zval.get_uuid())); break;

      case mtc::zval::z_charstr: {
        output->write("\"");
        output->write(*zval.get_charstr());
        output->write("\"");
        break;
      }
      case mtc::zval::z_widestr: {
        output->write("\"");
        output->write(to_utf8(*zval.get_widestr()));
        output->write("\"");
        break;
      }

      case mtc::zval::z_zmap:       serialize(output, *zval.get_zmap(), nullptr); break;
      case mtc::zval::z_array_zval: serialize(output, *zval.get_array_zval(), nullptr); break;

      default:
        LOG_W_C("Unsupported serialization type: %d", zval.get_type());
      }

      return output;
    }

    template<typename Output>
    auto serialize(Output *output, char value) -> Output *
    {
      std::string_view buf(&value, 1);
      return output->write(buf), output;
    }

    template<typename Output>
    auto serialize(Output *output, const mtc::zmap::key &key) -> Output *
    {
      return output->write(key.to_charstr()), output;
    }

    template<typename Output>
    auto serialize(Output *output, const mtc::zmap &zmap, const char *braces) -> Output *
    {
      const std::string_view braces_tmp = braces != nullptr ? braces : "";
      auto begin = std::begin(zmap);
      auto end = std::end(zmap);

      if (not braces_tmp.empty())
      {
        output = serialize(output, braces[0]);
      }
      if (begin == end)
      {
        return serialize(output, braces[1]);
      }

      if (begin->first == mtc::zmap::key{"\x1", 1})
      {
        return serialize(output, begin->second);
      }

      return serialize(serialize(serialize(serialize(serialize(output, '"'), begin->first), '"'), ':'), begin->second);
    }

    template<typename Output>
    auto serialize(Output *output, const mtc::array_zval &zvalues, const char *braces) -> Output *
    {
      const std::string_view braces_tmp = braces != nullptr ? braces : "";

      if (not braces_tmp.empty())
      {
        output = serialize(output, braces[0]);
      }

      for (auto iter = std::begin(zvalues); iter != std::end(zvalues); ++iter)
      {
        LOG_T_C("Serializing a value: [%d] %s", iter->get_type(), mtc::to_string(*iter).c_str());
        if (iter != std::begin(zvalues))
        {
          output = serialize(output, ',');
        }
        output = serialize(output, *iter);
      }

      if (not braces_tmp.empty() && braces_tmp.length() > 1)
      {
        output = serialize(output, braces[1]);
      }

      return output;
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
  void send_json_response(response_context<SSL> *ctx, const mtc::zmap &resp, std::string_view index, std::string_view docid)
  {
    assert(ctx != nullptr && "Invalid response context");
    if (ctx->aborted.load(std::memory_order_acquire)) {
      return;
    }

    if (ctx->response == nullptr) {
      return;
    }

    ctx->response->writeHeader("Content-Type", "application/json");

    const auto error = check_resp_on_error(resp);
    if (error.code != 0)
    {
      ctx->response->writeStatus(http::to_string(http::status_codes::BAD_REQUEST));
      ctx->response->writeHeader("Content-Length", get_error_context_length(index, docid));

      // Writing a body.
      ctx->response->write(R"({"_index": ")");
      ctx->response->write(index);
      ctx->response->write(R"(","_id": ")");
      ctx->response->write(docid);
      ctx->response->write(R"(","found": false})");
    }
    else
    {
      ctx->response->writeStatus(http::to_string(http::status_codes::OK));

      if (resp.get_word32("found", 0) == 0)
      {// Not found a document by id
        ctx->response->writeHeader("Content-Length", get_error_context_length(index, docid));

        // Writing a body.
        ctx->response->write(R"({"_index": ")");
        ctx->response->write(index);
        ctx->response->write(R"(","_id": ")");
        ctx->response->write(docid);
        ctx->response->write(R"(","found": false})");
      }
      else
      {
        // Getting a reference on items.
        const auto &items = resp.get_array_zmap("items", {});
        for (const auto &item : items)
        {
          ctx->response->write(R"({"_index": ")");
          ctx->response->write(item.get_zmap("extra", {}).get_charstr("_index", ""));
          ctx->response->write(R"(",)");

          ctx->response->write(R"("_id": ")");
          ctx->response->write(item.get_charstr("_id", item.get_zmap("extra", {}).get_charstr("_id", "")));
          ctx->response->write(R"(",)");

          ctx->response->write(R"("_version": )");
          ctx->response->write(std::to_string(item.get_zmap("extra", {}).get_int32("_version", -1)));
          ctx->response->write(R"(,)");

          ctx->response->write(R"("_seq_no": )");
          ctx->response->write(std::to_string(item.get_zmap("extra", {}).get_int32("_seq_no", 0)));
          ctx->response->write(R"(,)");

          ctx->response->write(R"("_primary_term": )");
          ctx->response->write(std::to_string(item.get_zmap("extra", {}).get_int32("_primary_term", 0)));
          ctx->response->write(R"(,)");

          ctx->response->write(R"("found": true)");

          if (item.get_array_zval("quote", {}).empty())
          {
            continue;
          }

          ctx->response->write(R"(,)");
          ctx->response->write(R"("_source": )");

          // Serializing response result.
          serialize(ctx->response, item.get_array_zval("quote", {}), "{}");
        }
      }
    }
    ctx->response->write(R"(})");
    ctx->response->end();
  }

  template<bool SSL>
  void send_error_response(response_context<SSL> *ctx, status_codes status, std::string_view error_type, std::string_view reason) {
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

    send_json_response(ctx, response);
  }
//-------------------------------------------------------------------------//
  template
  void send_json_response<false>(response_context<false> *, const service_response &);

  template
  void send_json_response<true>(response_context<true> *, const service_response &);

  template
  void send_json_response<false>(response_context<false> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_json_response<true>(response_context<true> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_error_response<false>(response_context<false> *, http::status_codes, std::string_view, std::string_view);

  template
  void send_error_response<true>(response_context<true> *, http::status_codes, std::string_view, std::string_view);
//-------------------------------------------------------------------------//
}// namespace elastic::http
