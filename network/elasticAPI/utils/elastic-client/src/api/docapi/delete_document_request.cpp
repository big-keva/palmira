#include "elastic/api/docapi/delete_document_request.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
#include <utility>
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  delete_document_request::delete_document_request(std::string index, std::string id)
    : doc_index(std::move(index)), doc_id(std::move(id))
  {
    if (this->doc_index.empty())
    {
        throw std::invalid_argument{"Document index must not be empty"};
    }

    if (this->doc_id.empty())
    {
        throw std::invalid_argument{"Document id must not be empty"};
    }
  }

  auto delete_document_request::routing(std::string value) -> delete_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"Routing value must not be empty"};
    }

    return this->doc_routing = std::move(value), *this;
  }

  auto delete_document_request::refresh(refresh_policies value) noexcept -> delete_document_request&
  {
    return this->doc_refresh = value, *this;
  }

  auto delete_document_request::wait_for_active_shards(std::string value) -> delete_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"wait_for_active_shards must not be empty"};
    }

    return this->doc_wait_for_active_shards = std::move(value), *this;
  }

  auto delete_document_request::version(std::uint64_t value) noexcept -> delete_document_request&
  {
    return this->doc_version = value, *this;
  }

  auto delete_document_request::version_type(docapi::version_types value) noexcept -> delete_document_request&
  {
    return this->doc_version_type = value, *this;
  }

  auto delete_document_request::if_sequence_number(std::uint64_t value) noexcept -> delete_document_request&
  {
    return this->doc_if_sequence_number = value, *this;
  }

  auto delete_document_request::if_primary_term(std::uint64_t value) noexcept -> delete_document_request&
  {
    return this->doc_if_primary_term = value, *this;
  }

  auto delete_document_request::timeout(std::chrono::milliseconds value) -> delete_document_request&
  {
    if (value <= std::chrono::milliseconds::zero())
    {
      throw std::invalid_argument{"Request timeout must be greater than zero"};
    }

    return this->request_timeout = value, *this;
  }

  auto delete_document_request::index() const noexcept -> std::string_view
  {
      return this->doc_index;
  }

  auto delete_document_request::id() const noexcept -> std::string_view
  {
      return this->doc_id;
  }

  auto delete_document_request::build() const -> http::request
  {
      http::request request{http::method_types::del};
      auto& endpoint = request.endpoint();

      endpoint.append(this->doc_index).append("_doc").append(this->doc_id);

      if (this->doc_routing.has_value())
      {
          endpoint.query("routing", this->doc_routing.value());
      }

      if (this->doc_refresh.has_value())
      {
          endpoint.query("refresh", to_string(this->doc_refresh.value()));
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

      if (this->doc_if_sequence_number.has_value())
      {
          endpoint.query("if_seq_no", std::to_string(this->doc_if_sequence_number.value()));
      }

      if (this->doc_if_primary_term.has_value())
      {
          endpoint.query("if_primary_term", std::to_string(this->doc_if_primary_term.value()));
      }

      return request.accept_json().timeout(this->request_timeout), request;
  }
//-------------------------------------------------------------------------//
  auto delete_document_request::to_string(refresh_policies value) noexcept -> std::string_view
  {
    switch (value)
    {
      case refresh_policies::disabled: return "false";
      case refresh_policies::immediate: return "true";
      case refresh_policies::wait_for: return "wait_for";
    }

    return "false";
  }

  auto delete_document_request::to_string(docapi::version_types value) noexcept -> std::string_view
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
