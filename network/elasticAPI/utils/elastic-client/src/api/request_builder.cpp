#include "elastic/api/request_builder.h"

#include <stdexcept>
#include <utility>
//-------------------------------------------------------------------------//
namespace elastic::api
{
//-------------------------------------------------------------------------//
  request_builder::request_builder(http::client& client, http::method_types method)
    : client_(&client), request_(method)
  {
  }

  request_builder::request_builder(request_builder&& other) noexcept
    : client_(other.client_), request_(std::move(other.request_)), executed_(other.executed_)
  {
    other.client_ = nullptr;
    other.executed_ = true;
  }

  auto request_builder::operator=(request_builder&& other) noexcept -> request_builder&
  {
    if (this == &other)
    {
      return *this;
    }

    client_ = other.client_;
    request_ = std::move(other.request_);
    executed_ = other.executed_;

    other.client_ = nullptr;
    other.executed_ = true;

    return *this;
  }

  auto request_builder::path(std::string_view value) -> request_builder&
  {
    ensure_active();
    return request_.endpoint().path(value), *this;
  }

  auto request_builder::segment(std::string_view value) -> request_builder&
  {
      ensure_active();
      return request_.endpoint().append(value), *this;
  }

  auto request_builder::query(std::string_view name, std::string_view value) -> request_builder&
  {
    ensure_active();
    return request_.endpoint().query(name, value), *this;
  }

  auto request_builder::query(std::string_view name, const char* value) -> request_builder&
  {
    if (value == nullptr)
    {
      throw std::invalid_argument("query parameter value must not be null");
    }

    return query(name, std::string_view{value});
  }

  auto request_builder::query(std::string_view name, bool value) -> request_builder&
  {
    return query(name, value ? std::string_view{"true"} : std::string_view{"false"});
  }

  auto request_builder::query(std::string_view name, std::int64_t value) -> request_builder&
  {
    return query(name, std::to_string(value));
  }

  auto request_builder::query(std::string_view name, std::uint64_t value) -> request_builder&
  {
    return query(name, std::to_string(value));
  }

  auto request_builder::body(std::string value) -> request_builder&
  {
    ensure_active();
    return request_.body(std::move(value)), *this;
  }

  auto request_builder::body(std::string_view value) -> request_builder&
  {
    ensure_active();
    return request_.body(value), *this;
  }

  auto request_builder::body(const char* value) -> request_builder&
  {
    if (value == nullptr)
    {
      throw std::invalid_argument("HTTP request body must not be null");
    }

    return body(std::string_view{value});
  }

  auto request_builder::clear_body() -> request_builder&
  {
    ensure_active();
    return request_.clear_body(), *this;
  }

  auto request_builder::header(std::string_view name, std::string_view value) -> request_builder&
  {
    ensure_active();
    return request_.header(name, value), *this;
  }

  auto request_builder::set_header(std::string_view name, std::string_view value) -> request_builder&
  {
    ensure_active();
    return request_.set_header(name, value), *this;
  }

  auto request_builder::remove_header(std::string_view name) -> request_builder&
  {
    ensure_active();
    return request_.remove_header(name), *this;
  }

  auto request_builder::accept_json() -> request_builder&
  {
    ensure_active();
    return request_.accept_json(), *this;
  }

  auto request_builder::content_type_json() -> request_builder&
  {
    ensure_active();
    return request_.content_type_json(), *this;
  }

  auto request_builder::timeout(std::chrono::milliseconds value) -> request_builder&
  {
    ensure_active();
    return request_.timeout(value), *this;
  }

  auto request_builder::timeout(std::int64_t milliseconds) -> request_builder&
  {
    return timeout(std::chrono::milliseconds{milliseconds});
  }

  auto request_builder::execute() -> http::response
  {
    ensure_active();

    // Builder одноразовый: состояние запроса нельзя случайно использовать повторно.
    executed_ = true;
    auto* client = client_;
    client_ = nullptr;

    return client->execute(request_);
  }

  auto request_builder::request() const noexcept -> const http::request&
  {
    return request_;
  }
//-------------------------------------------------------------------------//
  auto request_builder::ensure_active() const -> void
  {
    if (executed_ || client_ == nullptr)
    {
      throw std::logic_error("request_builder is no longer active; create a new builder from client");
    }
  }
//-------------------------------------------------------------------------//
} // namespace elastic::api
