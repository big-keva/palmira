#include "simd-json-load-document.h"

#include <utility>
//-------------------------------------------------------------------------//
#include "simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  auto load_document(std::string_view json, std::function<mtc::api<DeliriX::IText>()> onadd) -> void
  {
    simdjson::padded_string padded_json(json);
    simdjson::ondemand::value root;
    simdjson::ondemand::document document;
    //<!!!> parser is not thread-safe.
    thread_local simdjson::ondemand::parser parser;

    throw_if_error(parser.iterate(padded_json).get(document), "failed to parse JSON document");
    throw_if_error(document.get_value().get(root), "failed to read root JSON value");

    // Loading JSON document.
    load_document(root, std::move(onadd));
  }

  auto load_document(simdjson::ondemand::value value, std::function<mtc::api<DeliriX::IText>()> onadd) -> void
  {
    switch (value.type())
    {
    case simdjson::ondemand::json_type::array: {
      auto array = value.get_array().value();
      for (auto element : array)
      {
        load_document(element.value(), onadd);
      }
      break;
    }
    case simdjson::ondemand::json_type::object: {
      auto obj = value.get_object().value();
      auto tag = mtc::api<DeliriX::IText>();

      for (auto field : obj)
      {
        auto key = field.unescaped_key().value();
        //  Loading a new document from value.
        load_document(static_cast<simdjson::ondemand::value>(field.value()), [&]() {
          return (tag != nullptr ? tag : onadd())->AddMarkupTag(key);
        });
      }
      break;
    }
    case simdjson::ondemand::json_type::string: {
      auto str = value.get_string().value();
      if (not str.empty())
      {
        onadd()->AddBlock(DeliriX::IText::persistent, str);
      }
      break;
    }
    case simdjson::ondemand::json_type::number: {
      auto str = value.raw_json_token();
      if (not str.empty())
      {
        onadd()->AddBlock(DeliriX::IText::persistent, str);
      }
      break;
    }
    case simdjson::ondemand::json_type::boolean:
      onadd()->AddBlock(DeliriX::IText::persistent, value.get_bool().value() ? "true" : "false");
      break;
    case simdjson::ondemand::json_type::null:
      onadd()->AddBlock(DeliriX::IText::persistent, "null");
      break;
    }
  }
//-------------------------------------------------------------------------//
} // namespace elastic::json
