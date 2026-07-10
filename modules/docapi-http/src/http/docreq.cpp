#include "docreq.h"
//-------------------------------------------------------------------------//
#include <sstream>
//-------------------------------------------------------------------------//
namespace docapi::http {
//-------------------------------------------------------------------------//
  namespace {
//-------------------------------------------------------------------------//
    auto url_decode(std::string_view value)  -> std::string {
      std::string out;

      out.reserve(value.size());
      for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
          const auto hex = value.substr(i + 1, 2);
          const int decoded = std::stoi(std::string(hex), nullptr, 16);

          out.push_back(static_cast<char>(decoded));
          i += 2;
        } else if (value[i] == '+') {
          out.push_back(' ');
        } else {
          out.push_back(value[i]);
        }
      }
      return out;
    }

    auto parse_bool(const query_params_t &params, const std::string &key, bool default_value) -> bool {
      const auto found = params.find(key);
      if (found == std::end(params)) {
        return default_value;
      }

      return found->second == "true" || found->second == "1";
    }

    auto split(std::string_view value, char delim = ',') -> std::vector<std::string> {
      std::vector<std::string> result;
      std::string current;
      std::istringstream stream(value.data());

      while (std::getline(stream, current, delim)) {
        if (not current.empty()) {
          result.push_back(current);
        }
      }

      return result;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  auto document_get_options::as_zval() const noexcept -> mtc::zval {
    return {};
  }
//-------------------------------------------------------------------------//
  mtc::zmap parse_query(std::string_view query) {
    mtc::zmap params;

    while (not query.empty()) {
      const std::size_t amp = query.find('&');
      const std::string_view pair = query.substr(0, amp);
      const std::size_t eq = pair.find('=');

      if (eq == std::string_view::npos) {
        params.set_charstr(url_decode(pair), "");
      } else {
        params.set_charstr(url_decode(pair.substr(0, eq)), url_decode(pair.substr(eq + 1)));
      }

      if (amp == std::string_view::npos) {
        break;
      }

      query.remove_prefix(amp + 1);
    }

    return params;
  }

  document_get_options parse_get_options(const query_params_t &params) {
    document_get_options options;

    options.source_enabled = parse_bool(params, "_source", true);
    options.realtime = parse_bool(params, "realtime", true);
    options.refresh = parse_bool(params, "refresh", false);

    if (const auto iter = params.find("_source"); iter != std::end(params)) {
      if (iter->second != "true" && iter->second != "false" && iter->second != "1" && iter->second != "0") {
        options.source_includes = split(iter->second);
        options.source_enabled = true;
      }
    }

    if (const auto iter = params.find("_source_includes"); iter != std::end(params)) {
      options.source_includes = split(iter->second);
    }

    if (const auto iter = params.find("_source_excludes"); iter != std::end(params)) {
      options.source_excludes = split(iter->second);
    }

    if (const auto iter = params.find("routing"); iter != std::end(params)) {
      options.routing = iter->second;
    }

    if (const auto iter = params.find("preference"); iter != std::end(params)) {
      options.preference = iter->second;
    }

    if (const auto iter = params.find("stored_fields"); iter != std::end(params)) {
      options.stored_fields = iter->second;
    }

    if (const auto iter = params.find("version"); iter != std::end(params)) {
      options.version = iter->second;
    }

    if (const auto iter = params.find("version_type"); iter != std::end(params)) {
      options.version_type = iter->second;
    }

    return options;
  }

  auto get_query_param(const mtc::zmap &params, std::string_view name, std::optional<std::string> default_value /*= std::nullopt*/) -> std::optional<std::string> {
    const auto found = params.get_charstr(name.data());
    if (found != nullptr) {
      return found->c_str();
    }
    return default_value;
  }
//-------------------------------------------------------------------------//
} // namespace docapi::http
