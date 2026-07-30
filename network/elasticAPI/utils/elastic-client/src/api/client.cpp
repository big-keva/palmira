#include "elastic/api/client.h"
//-------------------------------------------------------------------------//
namespace elastic::api
{
//-------------------------------------------------------------------------//
  client::client(std::string base_url) : http(std::move(base_url))
  {
  }

  auto client::method(http::method_types request_method) -> request_builder
  {
    return request_builder{this->http, request_method};
  }

  auto client::get(std::string_view path /*= {}*/) -> request_builder
  {
    auto builder = method(http::method_types::get);
    if (not path.empty())
    {
      builder.path(path);
    }
    return builder;
  }

  auto client::post(std::string_view path /*= {}*/) -> request_builder
  {
    auto builder = method(http::method_types::post);
    if (not path.empty())
    {
      builder.path(path);
    }
    return builder;
  }

  auto client::put(std::string_view path /*= {}*/) -> request_builder
  {
    auto builder = method(http::method_types::put);
    if (not path.empty())
    {
      builder.path(path);
    }
    return builder;
  }

  auto client::del(std::string_view path /*= {}*/) -> request_builder
  {
    auto builder = method(http::method_types::del);
    if (not path.empty())
    {
        builder.path(path);
    }
    return builder;
  }

  auto client::index(std::string_view index_name) -> typed_request_builder<docapi::index_document_response>
  {
    validate_path_segment(index_name, "Document index must not be empty");

    auto builder = method(http::method_types::post);
    builder.segment(index_name)
           .segment("_doc")
           .accept_json()
           .content_type_json();

    return typed_request_builder<docapi::index_document_response>{std::move(builder)};
  }

  auto client::index(std::string_view index_name, std::string_view document_id) -> typed_request_builder<docapi::index_document_response>
  {
    validate_path_segment(index_name, "Document index must not be empty");
    validate_path_segment(document_id, "Document id must not be empty");

    auto builder = method(http::method_types::put);
    builder.segment(index_name)
           .segment("_doc")
           .segment(document_id)
           .accept_json()
           .content_type_json();

    return typed_request_builder<docapi::index_document_response>{std::move(builder)};
  }

  auto client::get_document(std::string_view index_name, std::string_view document_id) -> typed_request_builder<docapi::get_document_response>
  {
    validate_path_segment(index_name, "Document index must not be empty");
    validate_path_segment(document_id, "Document id must not be empty");

    auto builder = method(http::method_types::get);
    builder.segment(index_name)
           .segment("_doc")
           .segment(document_id)
           .accept_json();

    return typed_request_builder<docapi::get_document_response>{std::move(builder)};
  }

  auto client::delete_document(std::string_view index_name, std::string_view document_id) -> typed_request_builder<docapi::delete_document_response>
  {
    validate_path_segment(index_name, "Document index must not be empty");
    validate_path_segment(document_id, "Document id must not be empty");

    auto builder = method(http::method_types::del);
    builder.segment(index_name)
           .segment("_doc")
           .segment(document_id)
           .accept_json();

    return typed_request_builder<docapi::delete_document_response>{std::move(builder)};
  }

  auto client::execute(const http::request &request) -> http::response
  {
    return this->http.execute(request);
  }

  auto client::base_url(std::string value) -> client&
  {
    return this->http.base_url(std::move(value)), *this;
  }

  auto client::base_url() const -> std::string
  {
    return this->http.base_url();
  }
//-------------------------------------------------------------------------//
} // namespace elastic::api
