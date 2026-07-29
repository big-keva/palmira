#pragma once
//-------------------------------------------------------------------------//
#include <cstdint>
#include <string_view>
#include <utility>

#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  class delete_document_response final
  {
    http::response resp;

  public:
    explicit delete_document_response(http::response response) noexcept;

    delete_document_response(const delete_document_response&) = delete;
    auto operator=(const delete_document_response&) -> delete_document_response& = delete;

    delete_document_response(delete_document_response&&) noexcept = default;
    auto operator=(delete_document_response&&) noexcept -> delete_document_response& = default;

    ~delete_document_response() = default;

    [[nodiscard]] auto status() const noexcept -> std::uint16_t;
    [[nodiscard]] auto body() const noexcept -> std::string_view;

    [[nodiscard]] auto ok() const noexcept -> bool;
    [[nodiscard]] auto deleted() const noexcept -> bool;
    [[nodiscard]] auto not_found() const noexcept -> bool;
    [[nodiscard]] auto conflict() const noexcept -> bool;
    [[nodiscard]] auto client_error() const noexcept -> bool;
    [[nodiscard]] auto server_error() const noexcept -> bool;

    [[nodiscard]] auto is_json() const -> bool;
    [[nodiscard]] auto has_header(std::string_view name) const -> bool;
    [[nodiscard]] auto header(std::string_view name) const -> std::string_view;

    [[nodiscard]] auto transport() const noexcept -> const http::response&;
    [[nodiscard]] auto transport() noexcept -> http::response&;

    template<typename Visitor>
    auto parse_json(Visitor&& visitor) const -> void
    {
      this->resp.parse_json(std::forward<Visitor>(visitor));
    }
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
