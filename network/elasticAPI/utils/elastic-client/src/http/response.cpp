#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    auto trim(std::string_view value) noexcept -> std::string_view
    {
      while (not value.empty())
      {
        const auto ch = static_cast<unsigned char>(value.front());
        if (ch != ' ' && ch != '\t')
        {
          break;
        }
        value.remove_prefix(1U);
      }

      while (not value.empty())
      {
        const auto ch = static_cast<unsigned char>(value.back());
        if (ch != ' ' && ch != '\t')
        {
          break;
        }
        value.remove_suffix(1U);
      }

      return value;
    }

    auto contains_token(std::string_view value, std::string_view expected) noexcept -> bool
    {
      std::size_t offset = 0U;

      while (offset <= value.size())
      {
        const std::size_t delimiter = value.find(',', offset);
        const std::size_t length = delimiter == std::string_view::npos ? value.size() - offset : delimiter - offset;

        if (detail::iequals(trim(value.substr(offset, length)), expected))
        {
          return true;
        }

        if (delimiter == std::string_view::npos)
        {
          break;
        }

        offset = delimiter + 1U;
      }

      return false;
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  auto response::clear() noexcept -> void
  {
    this->response_status = 0;
    this->response_headers.clear();
    this->raw_body.clear();
    this->raw_headers.clear();
    this->padded_body = simdjson::padded_string();
    this->headers_parsed = false;
  }

  auto response::parse_headers() const -> void
  {
    this->response_headers.clear();

    std::string_view input(this->raw_headers.data(), this->raw_headers.size());
    std::size_t offset = 0U;
    bool inside_response_block = false;

    while (offset < input.size())
    {
      const std::size_t line_break = input.find("\r\n", offset);
      const std::size_t line_end = line_break == std::string_view::npos ? input.size() : line_break;
      const std::string_view line = input.substr(offset, line_end - offset);

      if (line.starts_with("HTTP/"))
      {
        // Новый status-line означает новый ответ. Заголовки предыдущего
        // промежуточного ответа или redirect-блока больше не актуальны.
        this->response_headers.clear();
        inside_response_block = true;
      }
      else if (line.empty())
      {
        inside_response_block = false;
      }
      else if (inside_response_block)
      {
        const std::size_t separator = line.find(':');
        if (separator != std::string_view::npos && separator != 0U)
        {
          const std::string_view name = trim(line.substr(0U, separator));
          const std::string_view value = trim(line.substr(separator + 1U));

          this->response_headers.add(name, value);
        }
      }

      if (line_break == std::string_view::npos)
      {
        break;
      }

      offset = line_break + 2U;
    }

    this->headers_parsed = true;
  }

  auto response::headers() const -> const response_headers_type_t &
  {
    if (not this->headers_parsed)
    {
      parse_headers();
    }

    return this->response_headers;
  }

  auto response::body() const noexcept -> std::string_view
  {
    return {this->raw_body.data(), this->raw_body.size()};
  }

  auto response::get_status() const noexcept -> std::uint16_t
  {
    if (this->response_status <= 0L)
    {
      return 0U;
    }

    constexpr auto maximum = static_cast<long>(std::numeric_limits<std::uint16_t>::max());
    if (this->response_status > maximum)
    {
      return std::numeric_limits<std::uint16_t>::max();
    }

    return static_cast<std::uint16_t>(this->response_status);
  }

  auto response::informational() const noexcept -> bool
  {
    return this->response_status >= 100L && this->response_status < 200L;
  }

  auto response::ok() const noexcept -> bool
  {
    return this->response_status >= 200L && this->response_status < 300L;
  }

  auto response::redirect() const noexcept -> bool
  {
    return this->response_status >= 300L && this->response_status < 400L;
  }

  auto response::client_error() const noexcept -> bool
  {
    return this->response_status >= 400L && this->response_status < 500L;
  }

  auto response::server_error() const noexcept -> bool
  {
    return this->response_status >= 500L && this->response_status < 600L;
  }

  auto response::has_header(std::string_view name) const -> bool
  {
    return this->headers().contains(name);
  }

  auto response::header(std::string_view name) const -> std::string_view
  {
    return this->headers().get(name);
  }

  auto response::is_json() const -> bool
  {
    constexpr std::string_view suffix = "+json";
    std::string_view content_type = trim(header("Content-Type"));
    const std::size_t parameters = content_type.find(';');
    if (parameters != std::string_view::npos)
    {
      content_type = trim(content_type.substr(0U, parameters));
    }

    if (detail::iequals(content_type, "application/json"))
    {
      return true;
    }

    return content_type.size() > suffix.size() && detail::iequals(content_type.substr(content_type.size() - suffix.size()), suffix);
  }

  auto response::is_chunked() const -> bool
  {
    return contains_token(header("Transfer-Encoding"), "chunked");
  }

  auto response::keep_alive() const -> bool
  {
    const std::string_view connection = header("Connection");
    if (contains_token(connection, "close"))
    {
      return false;
    }

    return true;
  }
//-------------------------------------------------------------------------//
  template<class onjson_t>
  auto response::parse_json(onjson_t &&onjson) const -> void
  {
    this->padded_body = simdjson::padded_string(this->raw_body.data(), this->raw_body.size());

    auto document_result = this->parser.iterate(this->padded_body);
    if (document_result.error() != simdjson::SUCCESS)
    {
      throw simdjson::simdjson_error(document_result.error());
    }

    std::invoke(std::forward<onjson_t>(onjson), document_result.value());
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http