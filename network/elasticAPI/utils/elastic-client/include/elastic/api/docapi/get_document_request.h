#pragma once
//-------------------------------------------------------------------------//
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
#include "elastic/http/request.h"
//-------------------------------------------------------------------------//
#include "get_document_response.h"
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  enum class version_type
  {
      internal,
      external,
      external_gte,
      force
  };

  class get_document_request final
  {
    std::string doc_index;
    std::string doc_id;

    std::optional<std::string> doc_routing;
    std::optional<std::string> doc_preference;
    std::optional<bool> doc_realtime;
    std::optional<bool> doc_refresh;
    std::optional<bool> doc_source;
    std::vector<std::string> doc_source_includes;
    std::vector<std::string> doc_source_excludes;
    std::vector<std::string> doc_stored_fields;
    std::optional<std::uint64_t> doc_version;
    std::optional<docapi::version_type> doc_version_type;
    std::chrono::milliseconds request_timeout{std::chrono::seconds{30}};

  public:
    using response_type = get_document_response;

    /**
     * Constructor.
     * @param index [in] - A document index.
     * @param id [in] - A document id.
     */
    get_document_request(std::string index, std::string id);

    get_document_request(const get_document_request&) = default;
    auto operator=(const get_document_request&) -> get_document_request& = default;

    get_document_request(get_document_request&&) noexcept = default;
    auto operator=(get_document_request&&) noexcept -> get_document_request& = default;

    /**
     * Destructor.
     */
    ~get_document_request() = default;

    auto routing(std::string value) -> get_document_request&;
    auto preference(std::string value) -> get_document_request&;
    auto realtime(bool value) noexcept -> get_document_request&;
    auto refresh(bool value) noexcept -> get_document_request&;
    auto source(bool value) noexcept -> get_document_request&;
    auto source_includes(std::vector<std::string> fields) -> get_document_request&;
    auto source_excludes(std::vector<std::string> fields) -> get_document_request&;
    auto stored_fields(std::vector<std::string> fields) -> get_document_request&;
    auto version(std::uint64_t value) noexcept -> get_document_request&;
    auto version_type(docapi::version_type value) noexcept -> get_document_request&;
    auto timeout(std::chrono::milliseconds value) -> get_document_request&;

    [[nodiscard]] auto index() const noexcept -> std::string_view;
    [[nodiscard]] auto id() const noexcept -> std::string_view;

    [[nodiscard]] auto build() const -> http::request;

  private:
      [[nodiscard]] static auto bool_value(bool value) noexcept -> std::string_view;
      [[nodiscard]] static auto join(const std::vector<std::string>& values) -> std::string;
      [[nodiscard]] static auto to_string(docapi::version_type value) noexcept -> std::string_view;
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
