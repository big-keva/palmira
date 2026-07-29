#include "elastic/api/docapi/index_document_request.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
#include <utility>
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  namespace
  {
//-------------------------------------------------------------------------//
    auto validate_common(std::string_view index, std::string_view document) -> void
    {
      if (index.empty())
      {
        throw std::invalid_argument{"Document index must not be empty"};
      }

      if (document.empty())
      {
        throw std::invalid_argument{"Document body must not be empty"};
      }
    }
//-------------------------------------------------------------------------//
  } // namespace
//-------------------------------------------------------------------------//
  index_document_request::index_document_request(std::string index, std::string document)
    : doc_index(std::move(index)), request_document(std::move(document))
  {
    validate_common(this->doc_index, this->request_document);
  }

  index_document_request::index_document_request(std::string index, std::string id, std::string document)
    : doc_index(std::move(index)), doc_id(std::move(id)), request_document(std::move(document))
  {
    validate_common(this->doc_index, this->request_document);

    if (this->doc_id->empty())
    {
      throw std::invalid_argument{"Document id must not be empty"};
    }
  }

  auto index_document_request::routing(std::string value) -> index_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"Routing value must not be empty"};
    }

    return this->doc_routing = std::move(value), *this;
  }

  auto index_document_request::pipeline(std::string value) -> index_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"Pipeline value must not be empty"};
    }

    return this->doc_pipeline = std::move(value), *this;
  }

  auto index_document_request::operation(index_operation value) noexcept -> index_document_request&
  {
    return this->doc_operation = value, *this;
  }

  auto index_document_request::refresh(refresh_policies value) noexcept -> index_document_request&
  {
    return this->doc_refresh = value, *this;
  }

  auto index_document_request::require_alias(bool value) noexcept -> index_document_request&
  {
    return this->doc_require_alias = value, *this;
  }

  auto index_document_request::wait_for_active_shards(std::string value) -> index_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"wait_for_active_shards must not be empty"};
    }

    return this->doc_wait_for_active_shards = std::move(value), *this;
  }

  auto index_document_request::version(std::uint64_t value) noexcept -> index_document_request&
  {
    return this->doc_version = value, *this;
  }

  auto index_document_request::version_type(docapi::version_types value) noexcept -> index_document_request&
  {
    return this->doc_version_type = value, *this;
  }

  auto index_document_request::timeout(std::chrono::milliseconds value) -> index_document_request&
  {
    if (value <= std::chrono::milliseconds::zero())
    {
      throw std::invalid_argument{"Request timeout must be greater than zero"};
    }

    return this->request_timeout = value, *this;
  }

  auto index_document_request::index() const noexcept -> std::string_view
  {
    return this->doc_index;
  }

  auto index_document_request::id() const noexcept -> std::optional<std::string_view>
  {
    if (not this->doc_id.has_value())
    {
      return std::nullopt;
    }

    return this->doc_id.value();
  }

  auto index_document_request::document() const noexcept -> std::string_view
  {
    return this->request_document;
  }

  auto index_document_request::build() const -> http::request
  {
    const auto request_method = this->doc_id.has_value() ? http::method_types::put : http::method_types::post;
    http::request request{request_method};

    request.endpoint().append(this->doc_index).append("_doc");
    if (this->doc_id.has_value())
    {
      request.endpoint().append(this->doc_id.value());
    }

    auto &endpoint = request.endpoint();
    if (this->doc_routing.has_value())
    {
      endpoint.query("routing", this->doc_routing.value());
    }

    if (this->doc_pipeline.has_value())
    {
      endpoint.query("pipeline", this->doc_pipeline.value());
    }

    if (this->doc_operation.has_value())
    {
      endpoint.query("op_type", to_string(this->doc_operation.value()));
    }

    if (this->doc_refresh.has_value())
    {
      endpoint.query("refresh", to_string(this->doc_refresh.value()));
    }

    if (this->doc_require_alias.has_value())
    {
      endpoint.query("require_alias", bool_value(this->doc_require_alias.value()));
    }

    if (this->doc_wait_for_active_shards.has_value())
    {
      endpoint.query("wait_for_active_shards", this->doc_wait_for_active_shards.value());
    }

    if (this->doc_version.has_value())
    {
      endpoint.query("version", std::to_string(this->doc_version.value()));
    }

    if (this->doc_version_type.has_value())
    {
      endpoint.query("version_type", to_string(this->doc_version_type.value()));
    }

    request.body(this->request_document).accept_json().content_type_json().timeout(this->request_timeout);
    return request;
  }

  auto index_document_request::bool_value(bool value) noexcept -> std::string_view
  {
    return value ? "true" : "false";
  }

  auto index_document_request::to_string(index_operation value) noexcept -> std::string_view
  {
    switch (value)
    {
      case index_operation::index: return "index";
      case index_operation::create: return "create";
    }

    return "index";
  }

  auto index_document_request::to_string(refresh_policies value) noexcept -> std::string_view
  {
    switch (value)
    {
      case refresh_policies::disabled: return "false";
      case refresh_policies::immediate: return "true";
      case refresh_policies::wait_for: return "wait_for";
    }

    return "false";
  }

  auto index_document_request::to_string(docapi::version_types value) noexcept -> std::string_view
  {
    switch (value)
    {
      case docapi::version_types::internal: return "internal";
      case docapi::version_types::external: return "external";
      case docapi::version_types::external_gte: return "external_gte";
      case docapi::version_types::force: return "force";
    }

    return "internal";
  }
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
