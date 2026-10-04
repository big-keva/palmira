#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <functional>
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace elastic::docapi::json
{
//-------------------------------------------------------------------------//
  enum class request_types : std::uint8_t
  {
    unknown = 0,
    index,
    create,
    remove,
    update
  };

  using bulk_params_t = std::map<std::string, std::string>;
  using on_bulk_request_t = std::function<void(request_types, const bulk_params_t &params, std::string body_doc)>;
//-------------------------------------------------------------------------//
  /**
   * Parses bulk request.
   * @param payload [in] - A bulk request.
   * @param onrequest [in] - A callback method on a request of bulk.
   */
  auto parse_bulk_request(std::string_view payload, const on_bulk_request_t& onrequest) -> void;
//-------------------------------------------------------------------------//
  auto to_string(request_types type) -> std::string_view;
  auto request_type_from(std::string_view type) -> request_types;
//-------------------------------------------------------------------------//
} // namespace elastic::docapi::json
