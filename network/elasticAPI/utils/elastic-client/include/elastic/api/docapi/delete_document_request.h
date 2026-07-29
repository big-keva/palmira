#pragma once
//-------------------------------------------------------------------------//
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "elastic/api/docapi/delete_document_response.h"
#include "elastic/api/docapi/document_options.h"
#include "elastic/http/request.h"
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  class delete_document_request final
  {
    std::string doc_index;
    std::string doc_id;
    std::optional<std::string> doc_routing;
    std::optional<refresh_policies> doc_refresh;
    std::optional<std::string> doc_wait_for_active_shards;
    std::optional<std::uint64_t> doc_version;
    std::optional<docapi::version_types> doc_version_type;
    std::optional<std::uint64_t> doc_if_sequence_number;
    std::optional<std::uint64_t> doc_if_primary_term;
    std::chrono::milliseconds request_timeout{std::chrono::milliseconds{30000}};

  public:
    using response_type = delete_document_response;

    delete_document_request(std::string index, std::string id);

    auto routing(std::string value) -> delete_document_request&;
    auto refresh(refresh_policies value) noexcept -> delete_document_request&;
    auto wait_for_active_shards(std::string value) -> delete_document_request&;
    auto version(std::uint64_t value) noexcept -> delete_document_request&;
    auto version_type(docapi::version_types value) noexcept -> delete_document_request&;
    auto if_sequence_number(std::uint64_t value) noexcept -> delete_document_request&;
    auto if_primary_term(std::uint64_t value) noexcept -> delete_document_request&;
    auto timeout(std::chrono::milliseconds value) -> delete_document_request&;

    [[nodiscard]] auto index() const noexcept -> std::string_view;
    [[nodiscard]] auto id() const noexcept -> std::string_view;
    [[nodiscard]] auto build() const -> http::request;

  private:
    [[nodiscard]] static auto to_string(refresh_policies value) noexcept -> std::string_view;
    [[nodiscard]] static auto to_string(docapi::version_types value) noexcept -> std::string_view;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
