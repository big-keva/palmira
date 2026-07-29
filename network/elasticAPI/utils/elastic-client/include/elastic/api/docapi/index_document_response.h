#pragma once
//-------------------------------------------------------------------------//
#include <cstdint>
#include <string_view>
#include <utility>
//-------------------------------------------------------------------------//
#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  class index_document_response final
  {
    http::response resp;

  public:
    explicit index_document_response(http::response response) noexcept;

    index_document_response(const index_document_response&) = delete;
    auto operator=(const index_document_response&) -> index_document_response& = delete;

    index_document_response(index_document_response&&) noexcept = default;
    auto operator=(index_document_response&&) noexcept -> index_document_response& = default;

    ~index_document_response() = default;

    [[nodiscard]] auto status() const noexcept -> std::uint16_t;
    [[nodiscard]] auto body() const noexcept -> std::string_view;

    [[nodiscard]] auto ok() const noexcept -> bool;
    [[nodiscard]] auto created() const noexcept -> bool;
    [[nodiscard]] auto updated() const noexcept -> bool;
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
