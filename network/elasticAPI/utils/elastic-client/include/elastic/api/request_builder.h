#pragma once
//-------------------------------------------------------------------------//
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include "elastic/http/client.h"
#include "elastic/http/request.h"
#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::api
{
//-------------------------------------------------------------------------//
  class request_builder final
  {
    http::client *client_{nullptr};
    http::request request_;

  public:
    request_builder(const request_builder &) = delete;
    auto operator=(const request_builder &) -> request_builder & = delete;

  public:
    /**
     * Constructor.
     * @param client [in] - HTTP client.
     * @param method [in] - A type of method.
     */
    explicit request_builder(http::client& client, http::method_types method);

    request_builder(request_builder &&other) noexcept;
    auto operator=(request_builder &&other) noexcept -> request_builder&;

    /**
     * Destructor.
     */
    ~request_builder() = default;

    auto path(std::string_view value) -> request_builder&;
    auto segment(std::string_view value) -> request_builder&;

    auto query(std::string_view name, std::string_view value) -> request_builder&;
    auto query(std::string_view name, const char* value) -> request_builder&;
    auto query(std::string_view name, bool value) -> request_builder&;
    auto query(std::string_view name, std::int64_t value) -> request_builder&;
    auto query(std::string_view name, std::uint64_t value) -> request_builder&;

    auto body(std::string value) -> request_builder&;
    auto body(std::string_view value) -> request_builder&;
    auto body(const char* value) -> request_builder&;
    auto clear_body() -> request_builder&;

    auto header(std::string_view name, std::string_view value) -> request_builder&;
    auto set_header(std::string_view name, std::string_view value) -> request_builder&;
    auto remove_header(std::string_view name) -> request_builder&;

    auto accept_json() -> request_builder&;
    auto content_type_json() -> request_builder&;

    auto timeout(std::chrono::milliseconds value) -> request_builder&;
    auto timeout(std::int64_t milliseconds) -> request_builder&;

    [[nodiscard]]
    auto execute() -> http::response;

    [[nodiscard]]
    auto request() const noexcept -> const http::request&;

  private:
    auto ensure_active() const -> void;

  private:
    bool executed_{false};
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api
