#include "../simd-json-mget-parser.h"
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
#include "../simd-json-get-parser.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
/*
    {
      "docs": [
        {
          "_index": "test",
          "_id": "1",
          "_routing": "",
          "_source": ""
        },
        {
          "_id": "2"
        }
      ]
    }
*/
//-------------------------------------------------------------------------//
    auto parse_mget_doc_object(mtc::array_zmap *docs, simdjson::ondemand::object object, std::string_view default_index) -> void
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
  auto parse_mget_request(std::string_view body, std::string_view default_index) -> palmira::SearchArgs
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
      {"get", mtc::array_zmap{
        //<???> {"id", id}
      }},
      {"quote", mtc::zmap{
        {"mode", "source"}
      }}
    };

    auto array_docs = args.query.get_zmap()->get_array_zmap("get");
    simdjson::padded_string padded(body);
    auto &context = elastic::json::get_thread_parser_context();

    simdjson::ondemand::document document;
    elastic::json::throw_if_error(context.parser.iterate(padded).get(document), "failed to parse mget body");

    simdjson::ondemand::object root;
    elastic::json::throw_if_error(document.get_object().get(root), "mget body must be object");

    for (auto field_result : root)
    {
      simdjson::ondemand::field field;
      elastic::json::throw_if_error(std::move(field_result).get(field), "failed to read mget root field");

      const std::string_view key = elastic::json::read_key(field);
      simdjson::ondemand::value value = field.value();

      if (key == "docs" || key == "ids")
      {
        simdjson::ondemand::array docs;
        elastic::json::throw_if_error(value.get_array().get(docs), "mget docs must be array");

        for (auto doc_result : docs)
        {
          simdjson::ondemand::value doc_value;
          elastic::json::throw_if_error(std::move(doc_result).get(doc_value), "failed to read mget docs item");

          simdjson::ondemand::object doc_object;
          elastic::json::throw_if_error(doc_value.get_object().get(doc_object), "mget docs item must be object");

          // Parsing a document.
          parse_mget_doc_object(array_docs, doc_object, default_index);
        }
      }
      else
      {
        LOG_W_C("Unsupported a document type: %s", std::string(key.data(), key.length()).c_str());
      }
    }

    return args;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
