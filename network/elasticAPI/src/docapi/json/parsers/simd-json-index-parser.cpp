#include "../simd-json-index-parser.h"
//-------------------------------------------------------------------------//
#include <DeliriX/DOM-dump.hpp>
//-------------------------------------------------------------------------//
#include "../../../common/utils.h"
//-------------------------------------------------------------------------//
#include "../../../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../../../json/simd-json-load-document.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  auto parse_index_request(std::string_view body, const mtc::zmap &params) -> palmira::InsertArgs
  {
    palmira::InsertArgs args;
    // Making a new unique document id.
    args.objectId = params.get_charstr("_id", "");
    args.uVersion = params.get_int16("_version", 1);

    if (args.objectId.empty())
    {
      args.objectId = elastic::make_uid();
    }

    // Copying options into metadata.
    args.metadata = mtc::zmap{
      {"_index", params.get_charstr("_index", "")},
      {"_id",   args.objectId},
      {"_version", static_cast<std::int64_t>(args.uVersion)},
      {"_started", params.get_int64("_started", 0)
    }};

    // Parsing request body as JSON.
    elastic::json::load_document(body, [&]() -> mtc::api<DeliriX::IText> {
      return &args.GetTextAPI();
    });

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
