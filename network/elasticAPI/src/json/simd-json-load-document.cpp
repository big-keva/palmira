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
      simdjson::ondemand::array array;
      throw_if_error(value.get_array().get(array), "failed to read JSON array");

      for (auto element : array)
      {
        simdjson::ondemand::value array_value;
        throw_if_error(element.get(array_value), "failed to read JSON array element");

        // Loading a document from value.
        load_document(array_value, onadd);
      }
      break;
    }
    case simdjson::ondemand::json_type::object: {
      simdjson::ondemand::object object;
      throw_if_error(value.get_object().get(object), "failed to read JSON object");

      auto tag = onadd();
      if (tag == nullptr)
      {
        throw (std::invalid_argument("making a new tag failed"));
      }

      for (auto field_result : object)
      {
        auto child = mtc::api<DeliriX::IText>();
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read object field");

        std::string_view key;
        throw_if_error(field.unescaped_key(false).get(key), "failed to read key");

        // Copying a key, because owner of buffer is simdjson.
        std::string owned_key(key);
        simdjson::ondemand::value field_value = field.value();

        //  Loading a new document from value.
        load_document(field_value, [&tag, &child, owned_key]() mutable -> mtc::api<DeliriX::IText> {
          if (child == nullptr)
          {
            child = tag->AddMarkupTag(owned_key);
          }

          return child;
        });
      }
      break;
    }
    case simdjson::ondemand::json_type::string: {
      std::string_view string_value;
      throw_if_error(value.get_string().get(string_value), "failed to read JSON string");

      auto tag = onadd();
      if (tag == nullptr)
      {
        throw (std::invalid_argument("making a new tag failed"));
      }

      tag->AddBlock(DeliriX::IText::persistent, string_value);
      break;
    }
    case simdjson::ondemand::json_type::number: {
      const auto raw_number = value.raw_json_token();
      auto tag = onadd();
      if (tag == nullptr)
      {
        throw (std::invalid_argument("making a new tag failed"));
      }

      tag->AddBlock(DeliriX::IText::persistent, raw_number);
      break;
    }
    case simdjson::ondemand::json_type::boolean: {
      bool boolean_value = false;
      throw_if_error(value.get_bool().get(boolean_value), "failed to read JSON boolean");

      auto tag = onadd();
      if (tag == nullptr)
      {
        throw (std::invalid_argument("making a new tag failed"));
      }

      tag->AddBlock(DeliriX::IText::persistent, boolean_value ? "true" : "false");
      break;
    }
    case simdjson::ondemand::json_type::null: {
      auto tag = onadd();
      if (tag == nullptr)
      {
        throw (std::invalid_argument("making a new tag failed"));
      }

      tag->AddBlock(DeliriX::IText::persistent, "null");
      break;
    }
    default:
      break;
    }
  }
//-------------------------------------------------------------------------//
} // namespace elastic::json
