#include "../simd-json-index-parser.h"
//-------------------------------------------------------------------------//
#include <common/module-utils.h>
//-------------------------------------------------------------------------//
#include "../../simd-json-visit.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic
{
//-------------------------------------------------------------------------//
auto parse_index_request(std::string_view body, const mtc::zmap &params)
    -> palmira::InsertArgs
{
  palmira::InsertArgs args;

  // Making a new unique document id.
  args.objectId = params.get_charstr("_id", palmira::modules::make_uid());
  args.uVersion = params.get_int16("_version", 1);

  // Copying options into metadata.
  args.metadata =
      mtc::zmap{{"_index", params.get_charstr("_index", "")},
                {"_id", args.objectId},
                {"_version", static_cast<std::int64_t>(args.uVersion)},
                {"_started", params.get_int64("_started", 0)}};

  // Parsing request body as JSON.
  json::visit_json_cb(
      body,
      [&](const json::json_visit_event &event) -> bool
      {
        std::fprintf(stdout, "%s\n", event.as_str().c_str());

        // Adding a new block.
        if (event.type == docapi::json::json_value_types::string &&
            not event.value.string_value.empty())
        {
          args.GetTextAPI()
              .AddMarkupTag({event.name.data(), event.name.size()})
              ->AddBlock(event.value.string_value.data(),
                         event.value.string_value.size());
        }
        else if (event.type == docapi::json::json_value_types::boolean)
        {
          args.GetTextAPI()
              .AddMarkupTag({event.name.data(), event.name.size()})
              ->AddBlock(event.value.bool_value ? "true" : "false");
        }
        else if (event.type == docapi::json::json_value_types::int64)
        {
          args.GetTextAPI()
              .AddMarkupTag({event.name.data(), event.name.size()})
              ->AddBlock(std::to_string(event.value.int64_value).c_str());
        }
        else if (event.type == docapi::json::json_value_types::uint64)
        {
          args.GetTextAPI()
              .AddMarkupTag({event.name.data(), event.name.size()})
              ->AddBlock(std::to_string(event.value.uint64_value).c_str());
        }
        else if (event.type == docapi::json::json_value_types::double_value)
        {
          args.GetTextAPI()
              .AddMarkupTag({event.name.data(), event.name.size()})
              ->AddBlock(std::to_string(event.value.double_value).c_str());
        }
        else if (event.type == docapi::json::json_value_types::datetime)
        {
          //<TODO> args.GetTextAPI().AddMarkupTag({event.name.data(),
          //event.name.size()})->AddBlock("");
        }
        else if (event.type == docapi::json::json_value_types::null_value)
        {
          args.GetTextAPI()
              .AddMarkupTag({event.name.data(), event.name.size()})
              ->AddBlock("");
        }

        return false;
      });

  return args;
}
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic
