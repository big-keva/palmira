#include "../simd-json-search-parser.h"
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include "../../simd-json-common.h"
#include "../../simd-json-errors.h"
//-------------------------------------------------------------------------//
#include "../search/simd-json-query-parser.h"
//-------------------------------------------------------------------------//
namespace docapi::json::elastic
{
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
auto parse_track_total_hits(simdjson::ondemand::value value,
                            search_request &request) -> void
{
  bool bool_value = false;

  if (not value.get_bool().get(bool_value))
  {
    request.track_total_hits_enabled = bool_value;
    request.track_total_hits_limit.reset();
    return;
  }

  simdjson::ondemand::number number;
  throw_if_error(value.get_number().get(number),
                 "track_total_hits must be boolean or non-negative integer");

  request.track_total_hits_enabled = true;

  if (number.get_number_type() ==
      simdjson::ondemand::number_type::signed_integer)
  {
    const auto signed_value = number.get_int64();

    if (signed_value < 0)
    {
      throw(parse_error("track_total_hits must be non-negative"));
    }

    request.track_total_hits_limit = static_cast<std::uint64_t>(signed_value);
    return;
  }

  if (number.get_number_type() ==
      simdjson::ondemand::number_type::unsigned_integer)
  {
    request.track_total_hits_limit = number.get_uint64();
    return;
  }

  throw(parse_error("track_total_hits must be boolean or integer"));
}

auto parse_sort(simdjson::ondemand::value value, search_request &request)
    -> void
{
  simdjson::ondemand::array array;
  throw_if_error(value.get_array().get(array), "sort must be array");

  for (auto item_result : array)
  {
    simdjson::ondemand::value item;
    throw_if_error(std::move(item_result).get(item),
                   "failed to read sort item");

    std::string_view sort_string;

    if (not item.get_string().get(sort_string))
    {
      request.sort.push_back(
          sort_clause{.field = copy_string(sort_string), .order = "asc"});
      continue;
    }

    simdjson::ondemand::object object;
    throw_if_error(item.get_object().get(object),
                   "sort item must be string or object");

    for (auto field_result : object)
    {
      simdjson::ondemand::field field;
      throw_if_error(std::move(field_result).get(field),
                     "failed to read sort field");

      sort_clause clause;
      clause.field = copy_string(read_key(field));

      simdjson::ondemand::value sort_value = field.value();

      std::string_view order_string;
      if (not sort_value.get_string().get(order_string))
      {
        clause.order = copy_string(order_string);
      }
      else
      {
        simdjson::ondemand::object params;
        throw_if_error(sort_value.get_object().get(params),
                       "sort field value must be string or object");

        for (auto param_result : params)
        {
          simdjson::ondemand::field param;
          throw_if_error(std::move(param_result).get(param),
                         "failed to read sort param");

          const auto key = read_key(param);
          if (key == "order")
          {
            clause.order =
                read_string_value(param.value(), "sort.order must be string");
          }
        }
      }

      if (clause.order != "asc" && clause.order != "desc")
      {
        throw(parse_error("sort.order must be asc or desc"));
      }

      request.sort.emplace_back(std::move(clause));
      break;
    }
  }
}

auto parse_docvalue_fields(simdjson::ondemand::value value,
                           search_request &request) -> void
{
  simdjson::ondemand::array array;
  throw_if_error(value.get_array().get(array), "docvalue_fields must be array");

  for (auto item_result : array)
  {
    simdjson::ondemand::value item;
    throw_if_error(std::move(item_result).get(item),
                   "failed to read docvalue_fields item");

    std::string_view string_item;
    if (not item.get_string().get(string_item))
    {
      request.docvalue_fields.push_back(
          field_request{.field = copy_string(string_item), .format = ""});
      continue;
    }

    simdjson::ondemand::object object;
    throw_if_error(item.get_object().get(object),
                   "docvalue_fields item must be string or object");

    field_request field;
    for (auto field_result : object)
    {
      simdjson::ondemand::field json_field;
      throw_if_error(std::move(field_result).get(json_field),
                     "failed to read docvalue field object");

      const auto key = read_key(json_field);
      if (key == "field")
      {
        field.field = read_string_value(json_field.value(),
                                        "docvalue_fields.field must be string");
      }
      else if (key == "format")
      {
        field.format = read_string_value(
            json_field.value(), "docvalue_fields.format must be string");
      }
    }

    if (field.field.empty())
    {
      throw(parse_error("docvalue_fields object requires field"));
    }

    request.docvalue_fields.emplace_back(std::move(field));
  }
}
//-------------------------------------------------------------------------//
} // namespace
//-------------------------------------------------------------------------//
search_request parse_search_request(std::string_view body,
                                    std::vector<std::string> indices)
{
  if (body.empty())
  {
    return {};
  }

  auto &context = get_thread_parser_context();
  simdjson::padded_string padded(body);
  simdjson::ondemand::document document;
  throw_if_error(context.parser.iterate(padded).get(document),
                 "failed to parse search body");

  simdjson::ondemand::object root;
  throw_if_error(document.get_object().get(root), "search body must be object");

  search_request request;
  request.indices = std::move(indices);

  for (auto field_result : root)
  {
    simdjson::ondemand::field field;
    throw_if_error(std::move(field_result).get(field),
                   "failed to read search root field");

    const std::string_view key = read_key(field);
    simdjson::ondemand::value value = field.value();

    if (key == "from")
    {
      request.from = read_non_negative_integer(value, "from");
    }
    else if (key == "size")
    {
      request.size = read_non_negative_integer(value, "size");
    }
    else if (key == "_source")
    {
      parse_source_value(value, request.source);
    }
    else if (key == "timeout")
    {
      request.timeout = copy_string(read_string(value));
    }
    else if (key == "explain")
    {
      request.explain = read_bool(value, "explain");
    }
    else if (key == "profile")
    {
      request.profile = read_bool(value, "profile");
    }
    else if (key == "track_scores")
    {
      request.track_scores = read_bool(value, "track_scores");
    }
    else if (key == "sort")
    {
      parse_sort(value, request);
    }
    else if (key == "stored_fields")
    {
      parse_string_array(value, request.stored_fields,
                         "stored_fields must be array");
    }
    else if (key == "fields")
    {
      parse_string_array(value, request.fields, "fields must be array");
    }
    else if (key == "docvalue_fields")
    {
      parse_docvalue_fields(value, request);
    }
    else if (key == "search_after")
    {
      parse_string_array(value, request.search_after,
                         "search_after must be array");
    }
    else if (key == "track_total_hits")
    {
      parse_track_total_hits(value, request);
    }
    else if (key == "track_scores")
    {
      request.track_scores =
          read_bool_value(value, "track_scores must be boolean");
    }
    else if (key == "version")
    {
      request.version = read_bool_value(value, "version must be boolean");
    }
    else if (key == "seq_no_primary_term")
    {
      request.seq_no_primary_term =
          read_bool_value(value, "seq_no_primary_term must be boolean");
    }
    else if (key == "terminate_after")
    {
      request.terminate_after =
          read_non_negative_integer(value, "terminate_after");
    }
    else if (key == "min_score")
    {
      request.min_score = read_double_value(value, "min_score must be double");
      request.has_min_score = true;
    }
    else if (key == "query")
    {
      simdjson::ondemand::object query_object;
      throw_if_error(value.get_object().get(query_object),
                     "query must be object");

      // Parsing a query.
      request.query = search::parse_query_object(query_object);
    }
  }

  return request;
}
//-------------------------------------------------------------------------//
} // namespace docapi::json::elastic
