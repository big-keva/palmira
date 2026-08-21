#include "http-req.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    const std::vector<std::string_view> g_headers = {
      {"host"},
      {"user-agent"},
      {"accept"},
      {"accept-encoding"},
      {"accept-language"},
      {"content-type"},
      {"content-length"},
      {"authorization"},
      {"origin"},
      {"referer"},
      {"connection"},
      {"cache-control"},
      {"pragma"},
      {"x-requested-with"},
      {"access-control-request-method"},
      {"access-control-request-headers"}
  };
//-------------------------------------------------------------------------//
    auto url_decode(std::string_view value)  -> std::string
    {
      std::string out;

      out.reserve(value.size());
      for (std::size_t i = 0; i < value.size(); ++i)
      {
        if (value[i] == '%' && i + 2 < value.size())
        {
          const auto hex = value.substr(i + 1, 2);
          const int decoded = std::stoi(std::string(hex), nullptr, 16);

          out.push_back(static_cast<char>(decoded));
          i += 2;
        }
        else if (value[i] == '+')
        {
          out.push_back(' ');
        }
        else
        {
          out.push_back(value[i]);
        }
      }
      return out;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  mtc::zmap parse_query(std::string_view query)
  {
    mtc::zmap params;

    while (not query.empty())
    {
      const std::size_t amp = query.find('&');
      const std::string_view pair = query.substr(0, amp);
      const std::size_t eq = pair.find('=');

      if (eq == std::string_view::npos)
      {
        params.set_charstr(url_decode(pair), "");
      }
      else
      {
        params.set_charstr(url_decode(pair.substr(0, eq)), url_decode(pair.substr(eq + 1)));
      }

      if (amp == std::string_view::npos)
      {
        break;
      }

      query.remove_prefix(amp + 1);
    }

    return params;
  }

  auto get_query_param(const mtc::zmap &params, std::string_view name, std::optional<std::string> default_value /*= std::nullopt*/) -> std::optional<std::string>
  {
    const auto found = params.get_charstr(name.data());
    if (found != nullptr)
    {
      return found->c_str();
    }
    return default_value;
  }
//-------------------------------------------------------------------------//
  auto get_supported_headers() -> const std::vector<std::string_view> &
  {
    return g_headers;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
