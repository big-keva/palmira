#include "../simd-json-get-parser.h"
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../../../json/simd-json-common.h"
#include "../../../json/simd-json-errors.h"
#include "../../../logger/logger.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    auto parse_get_doc_object(mtc::array_zmap *docs, simdjson::ondemand::object object, std::string_view default_index) -> void
    {
      auto zdoc = mtc::zmap{};

      if (not default_index.empty())
      {
        zdoc.set_charstr("_index", default_index.data());
      }

      for (auto field_result : object)
      {
        simdjson::ondemand::field field;
        elastic::json::throw_if_error(std::move(field_result).get(field), "failed to read mget doc field");

        const std::string_view key = elastic::json::read_key(field);
        simdjson::ondemand::value value = field.value();

        zdoc.set_charstr(key, elastic::json::read_string(value).data());
      }

      if (zdoc.empty())
      {
        LOG_E_C("Document is empty");
        return;
      }

      if (docs != nullptr)
      {
        // Adding a new document.
        docs->push_back(std::move(zdoc));
      }
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  auto parse_get_request(std::string_view index, std::string_view id, const mtc::zmap &params) -> palmira::SearchArgs
  {
    palmira::SearchArgs args;
    // Setting an order of searching.
    args.order = mtc::zmap{
      {"first", 1},
      {"count", 10},
      {"quote", mtc::zmap{
        {"mode", "source"}
      }}
    };
    args.query = mtc::zmap{
      {"get", mtc::zmap{
        {"id", std::string(id.data(), id.length())}
      }},
     {"quote", mtc::zmap{
       {"mode", "source"}
     }}
    };

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
