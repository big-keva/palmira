#include "elastic/http/endpoint.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  [[nodiscard]]
  auto is_unreserved(unsigned char value) noexcept -> bool
  {
      return (value >= 'A' && value <= 'Z') ||
             (value >= 'a' && value <= 'z') ||
             (value >= '0' && value <= '9') ||
             value == '-' ||
             value == '.' ||
             value == '_' ||
             value == '~';
}
//-------------------------------------------------------------------------//
} // namespace
//-------------------------------------------------------------------------//
  auto endpoint::clear() noexcept -> endpoint&
  {
      this->segments.clear();
      this->query_params.clear();
      return *this;
  }

  auto endpoint::reserve(std::size_t segment_count, std::size_t query_count) -> endpoint&
  {
      this->segments.reserve(segment_count);
      this->query_params.reserve(query_count);
      return *this;
  }

  auto endpoint::append(std::string_view segment) -> endpoint&
  {
      if (segment.empty())
      {
          throw std::invalid_argument("endpoint segment must not be empty");
      }

      return this->segments.emplace_back(segment), *this;
  }

  auto endpoint::path(std::string_view path_value) -> endpoint&
  {
      std::size_t position = 0;

      while (position < path_value.size())
      {
          while (position < path_value.size() && path_value[position] == '/')
          {
              ++position;
          }

          if (position == path_value.size())
          {
              break;
          }

          const std::size_t separator = path_value.find('/', position);
          const std::size_t end = separator == std::string_view::npos
              ? path_value.size()
              : separator;

          append(path_value.substr(position, end - position));
          position = end;
      }

      return *this;
  }

  auto endpoint::remove_last() noexcept -> endpoint&
  {
      if (!this->segments.empty())
      {
          this->segments.pop_back();
      }

      return *this;
  }

  auto endpoint::replace_last(std::string_view segment) -> endpoint&
  {
      if (segment.empty())
      {
          throw std::invalid_argument("endpoint segment must not be empty");
      }

      if (this->segments.empty())
      {
          throw std::logic_error("cannot replace segment in an empty endpoint");
      }

      return this->segments.back().assign(segment), *this;
  }

  auto endpoint::query(std::string_view name, std::string_view value) -> endpoint&
  {
      if (name.empty())
      {
          throw std::invalid_argument("query parameter name must not be empty");
      }

      return this->query_params.push_back(parameter{std::string{name}, std::string{value}}), *this;
  }

  auto endpoint::query_if(
      bool condition,
      std::string_view name,
      std::string_view value) -> endpoint&
  {
      if (condition)
      {
          query(name, value);
      }

      return *this;
  }

  auto endpoint::empty() const noexcept -> bool
  {
      return this->segments.empty() && this->query_params.empty();
  }

  auto endpoint::segment_count() const noexcept -> std::size_t
  {
      return this->segments.size();
  }

  auto endpoint::query_count() const noexcept -> std::size_t
  {
      return this->query_params.size();
  }

  auto endpoint::build() const -> std::string
  {
      std::size_t required_size = 1;

      for (const auto& segment : this->segments)
      {
          required_size += 1 + (segment.size() * 3);
      }

      for (const auto& parameter_value : this->query_params)
      {
          required_size += 2 +
                           (parameter_value.name.size() * 3) +
                           (parameter_value.value.size() * 3);
      }

      std::string result;
      result.reserve(required_size);

      if (this->segments.empty())
      {
          result.push_back('/');
      }
      else
      {
          for (const auto& segment : this->segments)
          {
              result.push_back('/');
              result.append(endpoint::encode_component(segment));
          }
      }

      bool first_parameter = true;

      for (const auto& parameter_value : this->query_params)
      {
          result.push_back(first_parameter ? '?' : '&');
          first_parameter = false;

          result.append(endpoint::encode_component(parameter_value.name));
          result.push_back('=');
          result.append(endpoint::encode_component(parameter_value.value));
      }

      return result;
  }
//-------------------------------------------------------------------------//
  auto endpoint::encode_component(std::string_view value) -> std::string
  {
      static constexpr char hex[] = "0123456789ABCDEF";

      std::string encoded;
      encoded.reserve(value.size());

      for (const unsigned char character : value)
      {
          if (is_unreserved(character))
          {
              encoded.push_back(static_cast<char>(character));
              continue;
          }

          encoded.push_back('%');
          encoded.push_back(hex[(character >> 4U) & 0x0FU]);
          encoded.push_back(hex[character & 0x0FU]);
      }

      return encoded;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
