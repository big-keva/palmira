#include "../simd-json-bulk-parser.h"
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../../../common/utils.h"
#include "../../../logger/logger.h"
//-------------------------------------------------------------------------//
#include "../../../json/simd-json-errors.h"
#include "../../../json/simd-json-common.h"
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  namespace
  {
    auto is_command_supported(request_types type) -> bool
    {
      return type != request_types::unknown;
    }

    auto parse_command_params(simdjson::ondemand::value value) -> bulk_params_t
    {
      bulk_params_t params;
      simdjson::ondemand::object object;
      elastic::json::throw_if_error(value.get_object().get(object), "body must be object");

      for (auto field_result : object)
      {
        simdjson::ondemand::field field;
        elastic::json::throw_if_error(std::move(field_result).get(field), "reading object failed");

        std::string_view field_name;
        elastic::json::throw_if_error(field.unescaped_key(false).get(field_name), "reading field name failed");

        std::string_view field_value;
        elastic::json::throw_if_error(field.value().get_string(field_value, false), "reading field value failed");

        // Adding a new name, value into params.
        params.emplace(field_name, field_value);
      }

      return params;
    }
  } // namespace
//-------------------------------------------------------------------------//
  auto parse_bulk_request(std::string_view payload, const on_bulk_request_t &onrequest) -> void
  {
    if (onrequest)
    {
      std::string json_cmd, json_doc;
      std::istringstream stream(payload.data());
      while (not stream.eof())
      {
        std::getline(stream, json_cmd);
        if (not json_cmd.empty())
        {
          simdjson::padded_string padded(json_cmd);
          auto &context = elastic::json::get_thread_parser_context();

          simdjson::ondemand::document document;
          elastic::json::throw_if_error(context.parser.iterate(padded).get(document), "parse a bulk body failed");

          simdjson::ondemand::object root;
          elastic::json::throw_if_error(document.get_object().get(root), "bulk body must be object");
          for (auto field_result : root)
          {
            simdjson::ondemand::field field;
            elastic::json::throw_if_error(std::move(field_result).get(field), "read bulk root field failed");

            const auto cmd_type = request_type_from(elastic::json::read_key(field));
            simdjson::ondemand::value value = field.value();
            if (not is_command_supported(cmd_type))
            {
              LOG_E_C("Unsupported command in a bulk request: %s", to_string(cmd_type).data());
              throw (elastic::json::parse_error("Unsupported command in a bulk request"));
            }

            auto cmd_params = parse_command_params(value);
            if (cmd_params.empty())
            {
              LOG_E_C("No parameters found or empty in command: no one parameter found in a bulk request");
              throw (elastic::json::parse_error("no one parameter found in a bulk request"));
            }

            if (cmd_type == request_types::index || cmd_type == request_types::update || cmd_type == request_types::create)
            {
              // Reading next line, which keeps a document.
              std::getline(stream, json_doc);
              if (json_doc.empty())
              {
                LOG_E_C("Invalid bulk payload format: after %s command (%s) must be a body",
                        to_string(cmd_type).data(), json_cmd.c_str());
                continue;
              }
            }

            // Raising READ command event.
            onrequest(cmd_type, cmd_params, std::move(json_doc));
          }
        }
      }
    }
  }
//-------------------------------------------------------------------------//
  auto to_string(request_types type) -> std::string_view
  {
    switch (type)
    {
      case request_types::index:   return "index";
      case request_types::update:  return "update";
      case request_types::remove:  return "delete";
      case request_types::create:  return "create";
      case request_types::unknown: return "unknown";
    }
    return "unknown";
  }

  auto request_type_from(std::string_view type) -> request_types
  {
    if (type == "create")
    {
      return request_types::create;
    }
    else if (type == "update")
    {
      return request_types::update;
    }
    else if (type == "delete")
    {
      return request_types::remove;
    }
    else if (type == "index")
    {
      return request_types::index;
    }
    return request_types::unknown;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
