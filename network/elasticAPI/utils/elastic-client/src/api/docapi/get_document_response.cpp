#include "elastic/api/docapi/get_document_response.h"
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  get_document_response::get_document_response(http::response response) noexcept
      : resp(std::move(response))
  {
  }

  auto get_document_response::status() const noexcept -> std::uint16_t
  {
      return this->resp.get_status();
  }

  auto get_document_response::body() const noexcept -> std::string_view
  {
      return this->resp.body();
  }

  auto get_document_response::ok() const noexcept -> bool
  {
      return this->resp.ok();
  }

  auto get_document_response::found() const noexcept -> bool
  {
      return status() == 200U;
  }

  auto get_document_response::not_found() const noexcept -> bool
  {
      return status() == 404U;
  }

  auto get_document_response::client_error() const noexcept -> bool
  {
      return this->resp.client_error();
  }

  auto get_document_response::server_error() const noexcept -> bool
  {
      return this->resp.server_error();
  }

  auto get_document_response::is_json() const -> bool
  {
      return this->resp.is_json();
  }

  auto get_document_response::has_header(std::string_view name) const -> bool
  {
      return this->resp.has_header(name);
  }

  auto get_document_response::header(std::string_view name) const -> std::string_view
  {
      return this->resp.header(name);
  }

  auto get_document_response::transport() const noexcept -> const http::response&
  {
      return this->resp;
  }

  auto get_document_response::transport() noexcept -> http::response&
  {
      return this->resp;
  }
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
