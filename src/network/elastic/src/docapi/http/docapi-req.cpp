#include "../docapi-req.h"
//-------------------------------------------------------------------------//
#include <sstream>
//-------------------------------------------------------------------------//
namespace elastic::http::docapi
{
//-------------------------------------------------------------------------//
  namespace {
//-------------------------------------------------------------------------//
    auto parse_bool(const query_params_t &params, const std::string &key, bool default_value) -> bool
    {
      const auto found = params.find(key);
      if (found == std::end(params))
      {
        return default_value;
      }

      return found->second == "true" || found->second == "1";
    }

    auto split(std::string_view value, char delim = ',') -> std::vector<std::string>
    {
      std::vector<std::string> result;
      std::string current;
      std::istringstream stream(value.data());

      while (std::getline(stream, current, delim))
      {
        if (not current.empty())
        {
          result.push_back(current);
        }
      }

      return result;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  auto document_get_options::as_zval() const noexcept -> mtc::zval
  {
    return {};
  }
//-------------------------------------------------------------------------//
  document_get_options parse_get_options(const query_params_t &params)
  {
    document_get_options options;

    options.source_enabled = parse_bool(params, "_source", true);
    options.realtime = parse_bool(params, "realtime", true);
    options.refresh = parse_bool(params, "refresh", false);

    if (const auto iter = params.find("_source"); iter != std::end(params))
    {
      if (iter->second != "true" && iter->second != "false" && iter->second != "1" && iter->second != "0")
      {
        options.source_includes = split(iter->second);
        options.source_enabled = true;
      }
    }

    if (const auto iter = params.find("_source_includes"); iter != std::end(params))
    {
      options.source_includes = split(iter->second);
    }

    if (const auto iter = params.find("_source_excludes"); iter != std::end(params))
    {
      options.source_excludes = split(iter->second);
    }

    if (const auto iter = params.find("routing"); iter != std::end(params))
    {
      options.routing = iter->second;
    }

    if (const auto iter = params.find("preference"); iter != std::end(params))
    {
      options.preference = iter->second;
    }

    if (const auto iter = params.find("stored_fields"); iter != std::end(params))
    {
      options.stored_fields = iter->second;
    }

    if (const auto iter = params.find("version"); iter != std::end(params))
    {
      options.version = iter->second;
    }

    if (const auto iter = params.find("version_type"); iter != std::end(params))
    {
      options.version_type = iter->second;
    }

    return options;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::docapi
