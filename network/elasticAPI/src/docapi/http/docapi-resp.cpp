#include "../../http/http-resp.h"
//-------------------------------------------------------------------------//
#include <string>
#include <filesystem>
//-------------------------------------------------------------------------//
#include <App.h>
//-------------------------------------------------------------------------//
#include "../../logger/logger.h"
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
      LOG_E_C("Proceed INDEX request failed: %s", exc.what());
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
    std::ostringstream buffer;

    try
    {
      LOG_T_C("Received response: %s", mtc::to_string(ret).c_str());
      const auto &resp = ret.get_zmap("resp", {});

      // Checking response on errors.
      check_resp_on_error(resp);

      //<TODO> Adding proceeding error in case index not found.

      if (resp.get_word32("found", 0) == 0)
      {// Not found document by id
        return R"({"_index": ")" + ret.get_charstr("_index", "") + R"(","_id": ")" + ret.get_charstr("_id", "") + R"(","found": false})";
      }

      // Getting a reference on items.
      const auto &items = resp.get_array_zmap("items", {});
      for (const auto &item : items)
      {
        buffer << R"("_index": )" << R"(")" << item.get_zmap("extra", {}).get_charstr("_index", "") << R"(",)" <<
                  R"("_id": )" << R"(")" << item.get_zmap("extra", {}).get_charstr("_id", "") << R"(",)" <<
                  R"("_version": )" << R"(")" << item.get_zmap("extra", {}).get_int32("_version", -1) << R"(",)" <<
                  R"("_seq_no": )" << R"(")" << item.get_zmap("extra", {}).get_int32("_seq_no", 0) << R"(",)" <<
                  R"("_primary_term": )" << R"(")" << item.get_zmap("extra", {}).get_int32("_primary_term", 0) << R"(",)" <<
                  R"("found": true,)" <<
                  R"("_source": {})";
      }
    }
    catch (const std::exception &exc)
    {
      LOG_E_C("Proceed SEARCH request failed: %s", exc.what());
    }

    return R"({)" + buffer.str() + R"(})";
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::docapi
