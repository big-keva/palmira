#include "../simd-json-mget-parser.h"
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../../../json/simd-json-errors.h"
#include "../../../json/simd-json-common.h"
//-------------------------------------------------------------------------//
#include "../../../searchapi/simd-json-search-parser.h"
//-------------------------------------------------------------------------//
namespace elastic::json::docapi
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    mget_doc parse_mget_doc_object(simdjson::ondemand::object object, std::string_view default_index)
    {
      mget_doc doc;
      if (not default_index.empty())
      {
        doc.index = json::copy_string(default_index);
      }

      for (auto field_result : object)
      {
        simdjson::ondemand::field field;
        throw_if_error(std::move(field_result).get(field), "failed to read mget doc field");

        const std::string_view key = read_key(field);
        simdjson::ondemand::value value = field.value();

        if (key == "_index")
        {
          doc.index = copy_string(read_string(value));
        }
        else if (key == "_id")
        {
          doc.id = copy_string(read_string(value));
        }
        else if (key == "routing" || key == "_routing")
        {
          doc.routing = copy_string(read_string(value));
        }
        else if (key == "_source")
        {
          parse_source_value(value, doc.source);
        }
      }

      if (doc.index.empty())
      {
        throw (parse_error("mget document has no _index"));
      }

      if (doc.id.empty())
      {
        throw (parse_error("mget document has no _id"));
      }

      return doc;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  mget_request parse_mget_request(std::string_view body, std::string_view default_index)
  {
    std::fprintf(stdout, "TRACE Received request payload: %s\n", body.data());

    mget_request request;
    simdjson::padded_string padded(body);
    auto &context = get_thread_parser_context();

    simdjson::ondemand::document document;
    throw_if_error(context.parser.iterate(padded).get(document), "failed to parse mget body");

    simdjson::ondemand::object root;
    throw_if_error(document.get_object().get(root), "mget body must be object");

    for (auto field_result : root)
    {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field), "failed to read mget root field");

      const std::string_view key = read_key(field);
      simdjson::ondemand::value value = field.value();

      if (key == "docs")
      {
        simdjson::ondemand::array docs;
        throw_if_error(value.get_array().get(docs), "mget docs must be array");

        for (auto doc_result : docs)
        {
          simdjson::ondemand::value doc_value;
          throw_if_error(std::move(doc_result).get(doc_value), "failed to read mget docs item");

          simdjson::ondemand::object doc_object;
          throw_if_error(doc_value.get_object().get(doc_object), "mget docs item must be object");

          request.docs.emplace_back(parse_mget_doc_object(doc_object, default_index));
        }
      }
      else if (key == "ids")
      {
        if (default_index.empty())
        {
          throw (parse_error("mget ids form requires index in URL"));
        }

        simdjson::ondemand::array ids;
        throw_if_error(value.get_array().get(ids), "mget ids must be array");

        for (auto id_result : ids)
        {
          simdjson::ondemand::value id_value;
          throw_if_error(std::move(id_result).get(id_value), "failed to read mget id");

          mget_doc doc;
          doc.index = copy_string(default_index);
          doc.id = copy_string(read_string(id_value));

          request.docs.emplace_back(std::move(doc));
        }
      }
    }

    if (request.docs.empty())
    {
      std::fprintf(stderr, "WARN mget request contains no documents\n");
      //<???> throw (parse_error("mget request contains no documents"));
    }

    return request;
  }

  auto parse_mget_request(const std::string &index, const std::string &id, const mtc::zmap &params) -> mget_request
  {
    mget_request request;
    request.docs.push_back(mget_doc{
      .index = index,
      .id = id,
      .routing = params.get_charstr("_routing", ""),
      .source = {}
    });
    return request;

  }
//-------------------------------------------------------------------------//
} // namespace elastic::json::docapi
