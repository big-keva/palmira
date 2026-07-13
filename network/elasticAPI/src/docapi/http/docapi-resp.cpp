#include "../../http/http-resp.h"
//-------------------------------------------------------------------------//
#include <string>
#include <filesystem>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "../docapi-resp.h"
//-------------------------------------------------------------------------//
namespace elastic::http::docapi
{
//-------------------------------------------------------------------------//
  auto make_index_response(const mtc::zmap &resp) -> std::string {
    std::ostringstream buffer;

    try
    {
      // Checking response on errors,
      check_resp_on_error(resp);

      // Getting metadata,
      const auto &mdata = resp.get_zmap("metadata", {});

      buffer << R"({)"
             << R"("_index": ")" + mdata.get_charstr("_index", "") + R"(",)"
             << R"("_id": ")" + mdata.get_charstr("_id", "") + R"(",)"
             << R"("_version": )" + std::to_string(mdata.get_int64("_version", 0)) + R"(,)"
             << R"("result": "created",)"
             << R"("_shards": {},)"
             << R"("_seq_no": 0,)"
             << R"("_primary_term": 1)"
             << R"(})";
    }
    catch (const std::exception &exc)
    {
      std::fprintf(stderr, "Proceed request failed: %s\n", exc.what());
    }
    return buffer.str();
  }

  auto make_update_response(const mtc::zmap &resp) -> std::string
  {
    return {};
  }

  auto make_remove_response(const mtc::zmap &resp) -> std::string
  {
    return {};
  }

  auto make_search_response(const mtc::zmap &ret) -> std::string
  {
    std::ostringstream _ids;

    try
    {
      std::fprintf(stdout, "TRACE Received response: %s\n", mtc::to_string(ret).c_str());
      const auto &resp = ret.get_zmap("resp", {});

      // Checking response on errors.
      check_resp_on_error(resp);

      //<TODO> Adding proceeding error in case index not found.

      if (resp.get_word32("found", 0) == 0)
      {// Not found document by id
        return R"({"_index": ")" + resp.get_charstr("_index", "") + R"(","_id": ")" + resp.get_charstr("_id", "") + R"(","found": true})";
      }

      // Getting a reference on items.
      const auto &items = resp.get_array_zmap("items", {});
      for (const auto &item : items)
      {
        _ids << R"("_id": )" << R"(")" << item.get_charstr("id", "") << R"(")";
      }
    }
    catch (const std::exception &exc)
    {
      std::fprintf(stderr, "[ERROR] %s\n", exc.what());
    }

    return R"({"_index": ")" + ret.get_charstr("_index", "") + R"(,)" + _ids.str() + R"(,"found": true})";
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::docapi
