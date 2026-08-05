#include "../../http/http-resp.h"
//-------------------------------------------------------------------------//
#include <string>
#include <filesystem>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include <mtc/json.h>
//-------------------------------------------------------------------------//
#include "../../logger/logger.h"
#include "../../json/serializer.h"
#include "../../common/utils.h"
//-------------------------------------------------------------------------//
#include "docapi-resp.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::http
{
//-------------------------------------------------------------------------//
  namespace
  {
    const std::unordered_map<unsigned, const std::string_view> g_braces_types = {
      {mtc::zval::z_char, ""},
      {mtc::zval::z_byte, ""},
      {mtc::zval::z_int16, ""},
      {mtc::zval::z_word16, ""},
      {mtc::zval::z_word32, ""},
      {mtc::zval::z_int64, ""},
      {mtc::zval::z_word64, ""},
      {mtc::zval::z_float, ""},
      {mtc::zval::z_double, ""},
      {mtc::zval::z_uuid, R"("")"},
      {mtc::zval::z_charstr, R"("")"},
      {mtc::zval::z_widestr, R"("")"},
      {mtc::zval::z_zmap, "{}"},

      {mtc::zval::z_array_char, "[]"},
      {mtc::zval::z_array_byte, "[]"},
      {mtc::zval::z_array_int16, "[]"},
      {mtc::zval::z_array_word16, "[]"},
      {mtc::zval::z_array_word32, "[]"},
      {mtc::zval::z_array_int64, "[]"},
      {mtc::zval::z_array_word64, "[]"},
      {mtc::zval::z_array_float, "[]"},
      {mtc::zval::z_array_double, "[]"},
      {mtc::zval::z_array_uuid, "[]"},
      {mtc::zval::z_array_charstr, "[]"},
      {mtc::zval::z_array_widestr, "[]"},
      {mtc::zval::z_array_zmap, "[]"},
      {mtc::zval::z_array_zval, "[]"},
    };
//-------------------------------------------------------------------------//
    template<typename Output>
    auto serialize(Output *output, const mtc::zmap &zmap, const char *braces = "") -> Output *;

    template<typename Output>
    auto serialize(Output *output, const mtc::array_zval &zvalues, const char *braces = "") -> Output *;

    template<typename Output>
    auto serialize(Output *output, char value) -> Output *;

    template<typename Output>
    auto serialize(Output *output, const mtc::zmap::key &key) -> Output *;
//-------------------------------------------------------------------------//
    template<typename Output>
    struct braces_guard
    {
      Output *output = nullptr;
      const char *braces = "";

      explicit braces_guard(Output *output_, const char *braces_)
        : output(output_), braces(braces_ != nullptr ? braces_ : "")
      {
        if (not this->empty())
        {
          serialize(output, this->braces[0]);
        }
      }

      ~braces_guard()
      {
        if (not this->empty())
        {
          serialize(output, this->braces[1]);
        }
      }

      [[nodiscard]]
      auto empty() const noexcept -> bool
      {
        return this->braces[0] == '\0';
      }
    };
//-------------------------------------------------------------------------//
    auto get_error_context_length(std::string_view index, std::string_view docid) -> size_t
    {
      static const auto s_length = std::strlen(R"({"_index": ")") +
                                              std::strlen(R"(","_id": ")") +
                                              std::strlen(R"(","found": false})");

      return s_length + index.length() + docid.length();
    }
//-------------------------------------------------------------------------//
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
    auto serialize(Output *output, const mtc::zval &zval, const char *braces = "") -> Output *
    {
      braces_guard guard(output, braces);

      switch (zval.get_type())
      {
      case mtc::zval::z_char:   output->write(std::to_string(*zval.get_char())); break;
      case mtc::zval::z_byte:   output->write(std::to_string(*zval.get_byte())); break;
      case mtc::zval::z_int16:  output->write(std::to_string(*zval.get_int16())); break;
      case mtc::zval::z_word16: output->write(std::to_string(*zval.get_word16())); break;
      case mtc::zval::z_int32:  output->write(std::to_string(*zval.get_int32())); break;
      case mtc::zval::z_word32: output->write(std::to_string(*zval.get_word32())); break;
      case mtc::zval::z_int64:  output->write(std::to_string(*zval.get_int64())); break;
      case mtc::zval::z_word64: output->write(std::to_string(*zval.get_word64())); break;
      case mtc::zval::z_float: {
        char buf[64] = {0};
        // Converting double into string.
        std::sprintf(buf, "%g", *zval.get_float());
        // Writing a string into response.
        output->write(buf);
        break;
      }
      case mtc::zval::z_double: {
        char buf[64] = {0};
        // Converting double into string.
        std::sprintf(buf, "%g", *zval.get_double());
        // Writing a string into response.
        output->write(buf);
        break;
      }
      case mtc::zval::z_bool:       output->write(*zval.get_bool() ? "true" : "false"); break;
      case mtc::zval::z_uuid: {
        braces_guard guard_uuid(output, guard.empty() ? g_braces_types.find(mtc::zval::z_uuid)->second.data() : "");

        output->write(mtc::to_string(*zval.get_uuid()));
        break;
      }
      case mtc::zval::z_charstr: {
        braces_guard guard_text(output, guard.empty() ? g_braces_types.find(mtc::zval::z_charstr)->second.data() : "");

        output->write(*zval.get_charstr());
        break;
      }
      case mtc::zval::z_widestr: {
        braces_guard guard_text(output, guard.empty() ? g_braces_types.find(mtc::zval::z_widestr)->second.data() : "");

        output->write(to_utf8(*zval.get_widestr()));
        break;
      }
      case mtc::zval::z_zmap:       output = serialize(output, *zval.get_zmap()); break;
      case mtc::zval::z_array_zval: output = serialize(output, *zval.get_array_zval()); break;
      default:
        LOG_W_C("Unsupported serialization type: %d", zval.get_type());
      }

      return output;
    }

    template<typename Output>
    auto serialize(Output *output, const mtc::zmap &zmap, const char *braces) -> Output *
    {
      auto begin = std::begin(zmap);
      auto end = std::end(zmap);
      auto need_quotes = [](unsigned type) -> bool {
        return type == mtc::zval::z_type::z_charstr || type == mtc::zval::z_type::z_widestr || type == mtc::zval::z_type::z_uuid;
      };
      braces_guard guard(output, braces);

      if (begin == end)
      {
        return output;
      }

      if (begin->first == mtc::zmap::key{"\x1", 1})
      {// There are items in one object.
        return serialize(output, begin->second, not need_quotes(begin->second.get_type()) ? "{}" : g_braces_types.find(begin->second.get_type())->second.data());
      }

      return serialize(serialize(serialize(serialize(serialize(output, '"'), begin->first), '"'), ':'),
                  begin->second,
                  g_braces_types.find(begin->second.get_type())->second.data());
    }

    template<typename Output>
    auto serialize(Output *output, const mtc::array_zval &zvalues, const char *braces) -> Output *
    {
      braces_guard guard(output, braces);

      for (auto iter = std::begin(zvalues); iter != std::end(zvalues); ++iter)
      {
        LOG_T_C("Serializing a value: [%d] %s", iter->get_type(), mtc::to_string(*iter).c_str());
        if (iter != std::begin(zvalues))
        {
          output = serialize(output, ',');
        }

        output = serialize(output, *iter);
      }

      return output;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  template<bool SSL>
  void send_index_json_response(uWS::HttpResponse<SSL> *reply, const mtc::zmap &resp, std::string_view index, std::string_view docid)
  {
    if (reply == nullptr) {
      return;
    }

    reply->writeHeader("Content-Type", "application/json");

    const auto error = elastic::http::check_resp_on_error(resp);
    if (error.code != 0)
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::BAD_REQUEST));
      reply->writeHeader("Content-Length", get_error_context_length(index, docid));

      braces_guard guard(reply, "{}");
      // Writing a body.
      reply->write(R"("_index": ")");
      reply->write(index);
      reply->write(R"(","_id": ")");
      reply->write(docid);
      reply->write(R"(","found": false)");
    }
    else
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::OK));

      braces_guard guard(reply, "{}");
      if (resp.get_zmap("status", {}).get_word16("code", 0) != 0)
      {// Not found a document by id
        reply->writeHeader("Content-Length", get_error_context_length(index, docid));

        // Writing a body.
        reply->write(R"("_index":")");
        reply->write(resp.get_charstr("_index", index.data()));
        reply->write(R"(","_id":")");
        reply->write(resp.get_charstr("_id", docid.data()));
        reply->write(R"(","found":false)");
      }
      else
      {
        reply->write(R"("_index": ")");
        reply->write(resp.get_charstr("_index", index.data()));
        reply->write(R"(",)");

        reply->write(R"("_id": ")");
        reply->write(resp.get_charstr("_id", docid.data()));
        reply->write(R"(",)");

        reply->write(R"("_version": )"); //<TODO> Need to include value into response from search engine.
        reply->write(std::to_string(resp.get_int64("_version", 1)));
        reply->write(R"(,)");

        reply->write(R"("result": "created",)");

        reply->write(R"("_shards": {},)"); //<TODO> Need to include value into response from search engine.

        reply->write(R"("_seq_no": )"); //<TODO> Need to include value into response from search engine.
        reply->write(std::to_string(resp.get_int64("_seq_no", 0LL)));
        reply->write(R"(,)");

        reply->write(R"("_primary_term": )"); //<TODO> Need to include value into response from search engine.
        reply->write(std::to_string(resp.get_int64("_primary_term", 0LL)));
        reply->write(R"(,)");
      }
    }

    reply->end();
  }

  template<bool SSL>
  void send_update_json_response(uWS::HttpResponse<SSL> *reply, const mtc::zmap &resp, std::string_view index, std::string_view docid)
  {
    if (reply == nullptr) {
      return;
    }

    reply->writeHeader("Content-Type", "application/json");

    const auto error = elastic::http::check_resp_on_error(resp);
    if (error.code != 0)
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::BAD_REQUEST));
      reply->writeHeader("Content-Length", get_error_context_length(index, docid));

      braces_guard guard(reply, "{}");
      // Writing a body.
      reply->write(R"("_index": ")");
      reply->write(index);
      reply->write(R"(","_id": ")");
      reply->write(docid);
      reply->write(R"(","found": false)");
    }
    else
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::OK));

      braces_guard guard(reply, "{}");
      // Getting metadata.
      const auto &mdata = resp.get_zmap("metadata", {});

      if (resp.get_zmap("status", {}).get_word16("code", 0) != 0)
      {// Not found a document by id
        reply->writeHeader("Content-Length", get_error_context_length(index, docid));

        // Writing a body.
        reply->write(R"("_index":")");
        reply->write(mdata.get_charstr("_index", index.data()));
        reply->write(R"(","_id":")");
        reply->write(mdata.get_charstr("_id", docid.data()));
        reply->write(R"(","found":false)");
      }
      else
      {
        reply->write(R"("_index": ")");
        reply->write(mdata.get_charstr("_index", index.data()));
        reply->write(R"(",)");

        reply->write(R"("_id": ")");
        reply->write(mdata.get_charstr("_id", docid.data()));
        reply->write(R"(",)");

        reply->write(R"("_version": )"); //<TODO> Need to include value into response from search engine.
        reply->write(std::to_string(mdata.get_int64("_version", 0LL)));
        reply->write(R"(,)");

        reply->write(R"("result": "created",)");

        reply->write(R"("_shards": {},)"); //<TODO> Need to include value into response from search engine.

        reply->write(R"("_seq_no": )"); //<TODO> Need to include value into response from search engine.
        reply->write(std::to_string(mdata.get_int64("_seq_no", 0LL)));
        reply->write(R"(,)");

        reply->write(R"("_primary_term": )"); //<TODO> Need to include value into response from search engine.
        reply->write(std::to_string(mdata.get_int64("_primary_term", 0LL)));
        reply->write(R"(,)");
      }
    }

    reply->end();
  }

  template<bool SSL>
  void send_get_json_response(uWS::HttpResponse<SSL> *reply, const mtc::zmap &resp, std::string_view index, std::string_view docid)
  {
    if (reply == nullptr) {
      return;
    }

    reply->writeHeader("Content-Type", "application/json");

    const auto error = elastic::http::check_resp_on_error(resp);
    if (error.code != 0)
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::BAD_REQUEST));
      reply->writeHeader("Content-Length", get_error_context_length(index, docid));

      braces_guard guard(reply, "{}");

      // Writing a body.
      reply->write(R"("_index": ")");
      reply->write(index);
      reply->write(R"(","_id": ")");
      reply->write(docid);
      reply->write(R"(","found": false)");
    }
    else
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::OK));

      braces_guard guard(reply, "{}");

      if (resp.get_word32("found", 0) == 0)
      {// Not found a document by id
        // Writing a body.
        reply->write(R"("_index":")");
        reply->write(index);
        reply->write(R"(","_id":")");
        reply->write(docid);
        reply->write(R"(","found":false)");
      }
      else
      {
        // Getting a reference on items.
        const auto &items = resp.get_array_zmap("items", {});
        for (const auto &item : items)
        {
          reply->write(R"("_index": ")");
          reply->write(item.get_zmap("extra", {}).get_charstr("_index", ""));
          reply->write(R"(",)");

          reply->write(R"("_id": ")");
          reply->write(item.get_charstr("_id", item.get_zmap("extra", {}).get_charstr("_id", "")));
          reply->write(R"(",)");

          reply->write(R"("_version": )");
          reply->write(std::to_string(item.get_zmap("extra", {}).get_int64("_version", 0LL)));
          reply->write(R"(,)");

          reply->write(R"("_seq_no": )");
          reply->write(std::to_string(item.get_zmap("extra", {}).get_int64("_seq_no", 0LL)));
          reply->write(R"(,)");

          reply->write(R"("_primary_term": )");
          reply->write(std::to_string(item.get_zmap("extra", {}).get_int64("_primary_term", 0LL)));
          reply->write(R"(,)");

          reply->write(R"("found": true)");

          if (item.get_array_zval("quote", {}).empty())
          {
            continue;
          }

          reply->write(R"(,)");
          reply->write(R"("_source": )");

          // Serializing response result.
          serialize(reply, item.get_array_zval("quote", {}));
        }
      }
    }
    reply->end();
  }

  template<bool SSL>
  void send_del_json_response(uWS::HttpResponse<SSL> *reply, const mtc::zmap &resp, std::string_view index, std::string_view docid)
  {
    if (reply == nullptr) {
      return;
    }
    reply->writeHeader("Content-Type", "application/json");

    const auto error = elastic::http::check_resp_on_error(resp);
    if (error.code != 0)
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::BAD_REQUEST));
      reply->writeHeader("Content-Length", get_error_context_length(index, docid));

      braces_guard guard(reply, "{}");
      // Writing a body.
      reply->write(R"("_index": ")");
      reply->write(index);
      reply->write(R"(","_id": ")");
      reply->write(docid);
      reply->write(R"(","found": false)");
    }
    else
    {
      reply->writeStatus(elastic::http::to_string(elastic::http::status_codes::OK));

      braces_guard guard(reply, "{}");

      if (resp.get_zmap("status", {}).get_word16("code", 0) != 0)
      {// Not found a document by id
        reply->writeHeader("Content-Length", get_error_context_length(index, docid));

        // Writing a body.
        reply->write(R"("_index":")");
        reply->write(index);
        reply->write(R"(","_id":")");
        reply->write(docid);
        reply->write(R"(","found":false)");
      }
      else
      {
        reply->write(R"("_index": ")");
        reply->write(index);
        reply->write(R"(",)");

        reply->write(R"("_id": ")");
        reply->write(docid);
        reply->write(R"(",)");

        reply->write(R"("_version": )");
        reply->write(std::to_string(resp.get_int64("_version", 0LL))); //<TODO> Need to include value into response from search engine.
        reply->write(R"(,)");

        reply->write(R"("result": "deleted",)");

        reply->write(R"("_shards": {},)"); //<TODO> Need to include value into response from search engine.

        reply->write(R"("_seq_no": )");
        reply->write(std::to_string(resp.get_int64("_seq_no", 0))); //<TODO> Need to include value into response from search engine.
        reply->write(R"(,)");

        reply->write(R"("_primary_term": )");
        reply->write(std::to_string(resp.get_int64("_primary_term", 0LL)));  //<TODO> Need to include value into response from search engine.
      }
    }

    reply->end();
  }
//-------------------------------------------------------------------------//
  template
  void send_index_json_response<false>(uWS::HttpResponse<false> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_index_json_response<true>(uWS::HttpResponse<true> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_update_json_response<false>(uWS::HttpResponse<false> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_update_json_response<true>(uWS::HttpResponse<true> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_get_json_response<false>(uWS::HttpResponse<false> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_get_json_response<true>(uWS::HttpResponse<true> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_del_json_response<false>(uWS::HttpResponse<false> *, const mtc::zmap &, std::string_view, std::string_view);

  template
  void send_del_json_response<true>(uWS::HttpResponse<true> *, const mtc::zmap &, std::string_view, std::string_view);
//-------------------------------------------------------------------------//
} // namespace elasti::docapic::http
