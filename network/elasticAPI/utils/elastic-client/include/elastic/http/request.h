#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <chrono>
//-------------------------------------------------------------------------//
#include "endpoint.h"
#include "headers.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  enum class method_types : std::uint8_t
  {
    get,
    post,
    put,
    del,
    head
  };

  struct request_header
  {
    std::string_view name;
    std::string_view value;

    /**
     * Constructor.
     * @param header_name [in] - A name of header.
     * @param header_value [in] - A value of header.
     */
    explicit request_header(std::string_view header_name, std::string_view header_value)
      : name(header_name), value(header_value)
    {
    }
  };
//-------------------------------------------------------------------------//
  class request
  {
    //!< Keeps request method.
    method_types request_method = method_types::get;

    //!< Keeps endpoint.
    http::endpoint server_endpoint;

    //!< Keeps a list of request headers.
    http::headers<request_header> request_headers;

    //!< Keeps a list of query parameters.
    std::string payload;

    //!< Keeps a request timeout.
    std::chrono::milliseconds request_timeout{30000};

  public:
    request() = default;

    /**
     * Constructor.
     * @param method_type [in] - A type of request method.
     */
    explicit request(method_types method_type) noexcept;

  public:
    auto method(enum method_types request_method) noexcept -> request &;
    [[nodiscard]] auto method() const noexcept -> method_types;

    auto endpoint(const http::endpoint &value) -> request &;
    auto endpoint(http::endpoint &&value) noexcept -> request &;

    auto body(std::string value) -> request &;
    auto body(std::string_view value) -> request &;
    auto clear_body() noexcept -> request &;

    auto header(std::string_view name, std::string_view value) -> request &;
    auto set_header(std::string_view name, std::string_view value) -> request &;
    auto remove_header(std::string_view name) -> request &;
    auto clear_headers() noexcept -> request &;

    auto accept_json() -> request &;
    auto content_type_json() -> request &;

    auto timeout(std::chrono::milliseconds value) -> request &;

  public:
    [[nodiscard]] auto endpoint() noexcept -> http::endpoint &;
    [[nodiscard]] auto endpoint() const noexcept -> const http::endpoint &;
    [[nodiscard]] auto target() const -> std::string;

    [[nodiscard]] auto body() const noexcept -> const std::string &;
    [[nodiscard]] auto has_body() const noexcept -> bool;

    [[nodiscard]] auto headers() noexcept -> http::headers<request_header> &;
    [[nodiscard]] auto headers() const noexcept -> const http::headers<request_header> &;

    [[nodiscard]] auto timeout() const noexcept -> std::chrono::milliseconds;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http