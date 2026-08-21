#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <optional>
#include <sstream>
#include <unordered_map>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
#include "network/elasticAPI/src/logger/logger.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  using query_params_t = std::unordered_map<std::string, std::string>;
//-------------------------------------------------------------------------//
  /**
   * Parses query string into a map of key-value pairs.
   * @param query [in] - A query string to parse.
   * @return A map of key-value pairs.
   */
  mtc::zmap parse_query(std::string_view query);

  /**
   * Gets a value of parameter by its name.
   * @param params [in] - A list of request parameters.
   * @param name [in] - Parameter name.
   * @param default_value [in] - Default value to return if parameter is not found.
   * @return Parameter value or default value if parameter is not found.
   */
  auto get_query_param(const mtc::zmap &params, std::string_view name, std::optional<std::string> default_value = std::nullopt) -> std::optional<std::string>;

  /**
   *
   * @return
   */
  auto get_supported_headers() -> const std::vector<std::string_view> &;
//-------------------------------------------------------------------------//
  template<typename request_t>
  auto dump_request(request_t *req) -> void
  {
    std::ostringstream buf;
    const auto &headers = get_supported_headers();

    buf << "============================================================" << std::endl
        << "UNHANDLED HTTP REQUEST" << std::endl
        << "------------------------------------------------------------" << std::endl
        << "Method    : " << std::string(req->getMethod().data(), req->getMethod().size()) << std::endl
        << "URL       : " << std::string(req->getUrl().data(), req->getUrl().size()) << std::endl
        << "Full URL  : " << std::string(req->getFullUrl().data(), req->getFullUrl().size()) << std::endl
        << "Query     : " << std::string(req->getQuery().data(), req->getQuery().size()) << std::endl;

    buf << "------------------------------------------------------------" << std::endl
        << "Headers: " << std::endl;

    for (const auto name : headers)
    {
      const auto value = req->getHeader(name);
      if (not value.empty())
      {
        buf << std::string(name.data(), name.size()) << ": " << std::string(value.data(), value.size()) << std::endl;
      }
    }
    buf << "============================================================";

    LOG_T_C("Dump request:\n%s", buf.str().c_str());
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
