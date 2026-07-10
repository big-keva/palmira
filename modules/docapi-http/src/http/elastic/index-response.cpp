#include "../docres.h"
//-------------------------------------------------------------------------//
#include <sstream>
//-------------------------------------------------------------------------//
#include <common/errors.h>
//-------------------------------------------------------------------------//
namespace docapi::http::elastic {
//-------------------------------------------------------------------------//
  auto make_index_response(const mtc::zmap &resp) -> std::string {
    std::ostringstream buffer;

    try {
      // Checking response on errors,
      palmira::modules::check_errors(resp);

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
    } catch (const palmira::modules::palmira_error &exc) {
      std::fprintf(stderr, "Proceed request failed: (%d) %s\n", exc.code(), exc.what());
    }
    return buffer.str();
  }
//-------------------------------------------------------------------------//
} // namespace docapi::http::elastic