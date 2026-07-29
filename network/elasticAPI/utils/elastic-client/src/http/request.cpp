#include "elastic/http/request.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  request::request(method_types method_type) noexcept : request_method(method_type)
  {
  }
//-------------------------------------------------------------------------//
  auto request::method(method_types method_type) noexcept -> request &
  {
    return this->request_method = method_type, *this;
  }

  auto request::method() const noexcept -> method_types
  {
    return this->request_method;
  }

  auto request::endpoint(const http::endpoint &value) -> request &
  {
    return this->server_endpoint = value, *this;
  }

  auto request::endpoint(http::endpoint &&value) noexcept -> request &
  {
    return this->server_endpoint = std::move(value), *this;
  }

  auto request::body(std::string value) -> request &
  {
    return this->payload = std::move(value), *this;
  }

  auto request::body(std::string_view value) -> request &
  {
    return this->payload.assign(value.data(), value.size()), *this;
  }

  auto request::clear_body() noexcept -> request &
  {
    return this->payload.clear(), *this;
  }

  auto request::header(std::string_view name, std::string_view value) -> request &
  {
    if (name.empty())
    {
      throw std::invalid_argument("HTTP header name must not be empty");
    }

    return this->request_headers.add(name, value), *this;
  }

  auto request::set_header(std::string_view name, std::string_view value) -> request &
  {
    if (name.empty())
    {
      throw std::invalid_argument("HTTP header name must not be empty");
    }

    return this->request_headers.set(name, value), *this;
  }

  auto request::remove_header(std::string_view name) -> request &
  {
    return this->request_headers.remove(name), *this;
  }

  auto request::clear_headers() noexcept -> request &
  {
    return this->request_headers.clear(), *this;
  }

  auto request::accept_json() -> request &
  {
    return this->set_header("Accept", "application/json");
  }

  auto request::content_type_json() -> request &
  {
    return this->set_header("Content-Type", "application/json");
  }

  auto request::timeout(std::chrono::milliseconds value) -> request &
  {
    if (value.count() <= 0)
    {
      throw std::invalid_argument("HTTP request timeout must be greater than zero");
    }

    return this->request_timeout = value, *this;
  }
//-------------------------------------------------------------------------//
  auto request::endpoint() noexcept -> http::endpoint &
  {
    return this->server_endpoint;
  }

  auto request::endpoint() const noexcept -> const http::endpoint &
  {
    return this->server_endpoint;
  }

  auto request::target() const -> std::string
  {
    return this->server_endpoint.build();
  }

  auto request::body() const noexcept -> const std::string &
  {
    return this->payload;
  }

  auto request::has_body() const noexcept -> bool
  {
    return !this->payload.empty();
  }

  auto request::headers() noexcept -> http::headers<request_header> &
  {
    return this->request_headers;
  }

  auto request::headers() const noexcept -> const http::headers<request_header> &
  {
    return this->request_headers;
  }

  auto request::timeout() const noexcept -> std::chrono::milliseconds
  {
    return this->request_timeout;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http
