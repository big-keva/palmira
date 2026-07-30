#pragma once

/*
 * История изменений:
 * 2026-07-30 — добавлена типизированная fluent-обёртка над request_builder.
 */

#include <chrono>
#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "elastic/api/request_builder.h"
#include "elastic/http/response.h"

namespace elastic::api
{

template<class Response>
concept response_from_http =
    std::constructible_from<Response, http::response>;

template<response_from_http Response>
class typed_request_builder final
{
public:
    explicit typed_request_builder(request_builder builder) noexcept
        : builder_(std::move(builder))
    {
    }

    typed_request_builder(const typed_request_builder&) = delete;
    auto operator=(const typed_request_builder&) -> typed_request_builder& = delete;

    typed_request_builder(typed_request_builder&&) noexcept = default;
    auto operator=(typed_request_builder&&) noexcept
        -> typed_request_builder& = default;

    ~typed_request_builder() = default;

    auto query(std::string_view name, std::string_view value)
        -> typed_request_builder&
    {
        builder_.query(name, value);
        return *this;
    }

    auto query(std::string_view name, const char* value)
        -> typed_request_builder&
    {
        builder_.query(name, value);
        return *this;
    }

    auto query(std::string_view name, bool value)
        -> typed_request_builder&
    {
        builder_.query(name, value);
        return *this;
    }

    auto query(std::string_view name, std::int64_t value)
        -> typed_request_builder&
    {
        builder_.query(name, value);
        return *this;
    }

    auto query(std::string_view name, std::uint64_t value)
        -> typed_request_builder&
    {
        builder_.query(name, value);
        return *this;
    }

    auto body(std::string value) -> typed_request_builder&
    {
        builder_.body(std::move(value));
        return *this;
    }

    auto body(std::string_view value) -> typed_request_builder&
    {
        builder_.body(value);
        return *this;
    }

    auto body(const char* value) -> typed_request_builder&
    {
        builder_.body(value);
        return *this;
    }

    auto clear_body() -> typed_request_builder&
    {
        builder_.clear_body();
        return *this;
    }

    auto header(std::string_view name, std::string_view value)
        -> typed_request_builder&
    {
        builder_.header(name, value);
        return *this;
    }

    auto set_header(std::string_view name, std::string_view value)
        -> typed_request_builder&
    {
        builder_.set_header(name, value);
        return *this;
    }

    auto remove_header(std::string_view name) -> typed_request_builder&
    {
        builder_.remove_header(name);
        return *this;
    }

    auto accept_json() -> typed_request_builder&
    {
        builder_.accept_json();
        return *this;
    }

    auto content_type_json() -> typed_request_builder&
    {
        builder_.content_type_json();
        return *this;
    }

    auto timeout(std::chrono::milliseconds value) -> typed_request_builder&
    {
        builder_.timeout(value);
        return *this;
    }

    auto timeout(std::int64_t milliseconds) -> typed_request_builder&
    {
        builder_.timeout(milliseconds);
        return *this;
    }

    [[nodiscard]]
    auto execute() -> Response
    {
        return Response{builder_.execute()};
    }

    [[nodiscard]]
    auto request() const noexcept -> const http::request&
    {
        return builder_.request();
    }

private:
    request_builder builder_;
};

} // namespace elastic::api
