#pragma once
//-------------------------------------------------------------------------//
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "elastic/api/docapi/document_options.h"
#include "elastic/api/docapi/index_document_response.h"
#include "elastic/http/request.h"
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  enum class index_operation {index, create};
//-------------------------------------------------------------------------//
  class index_document_request final
  {
    std::string doc_index;
    std::optional<std::string> doc_id;
    std::string request_document;

    std::optional<std::string> doc_routing;
    std::optional<std::string> doc_pipeline;
    std::optional<index_operation> doc_operation;
    std::optional<refresh_policies> doc_refresh;
    std::optional<bool> doc_require_alias;
    std::optional<std::string> doc_wait_for_active_shards;
    std::optional<std::uint64_t> doc_version;
    std::optional<docapi::version_types> doc_version_type;
    std::chrono::milliseconds request_timeout{std::chrono::milliseconds{30000}};

  public:
      using response_type = index_document_response;

      index_document_request(std::string index, std::string document);
      index_document_request(std::string index, std::string id, std::string document);

      index_document_request(const index_document_request&) = default;
      auto operator=(const index_document_request&) -> index_document_request& = default;

      index_document_request(index_document_request&&) noexcept = default;
      auto operator=(index_document_request&&) noexcept -> index_document_request& = default;

      ~index_document_request() = default;

      auto routing(std::string value) -> index_document_request&;
      auto pipeline(std::string value) -> index_document_request&;
      auto operation(index_operation value) noexcept -> index_document_request&;
      auto refresh(refresh_policies value) noexcept -> index_document_request&;
      auto require_alias(bool value) noexcept -> index_document_request&;
      auto wait_for_active_shards(std::string value) -> index_document_request&;
      auto version(std::uint64_t value) noexcept -> index_document_request&;
      auto version_type(docapi::version_types value) noexcept -> index_document_request&;
      auto timeout(std::chrono::milliseconds value) -> index_document_request&;

      [[nodiscard]] auto index() const noexcept -> std::string_view;
      [[nodiscard]] auto id() const noexcept -> std::optional<std::string_view>;
      [[nodiscard]] auto document() const noexcept -> std::string_view;

      [[nodiscard]] auto build() const -> http::request;

  private:
      [[nodiscard]] static auto bool_value(bool value) noexcept -> std::string_view;
      [[nodiscard]] static auto to_string(index_operation value) noexcept -> std::string_view;
      [[nodiscard]] static auto to_string(refresh_policies value) noexcept -> std::string_view;
      [[nodiscard]] static auto to_string(docapi::version_types value) noexcept -> std::string_view;
};
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
