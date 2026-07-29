#include "elastic/api/docapi/get_document_request.h"
//-------------------------------------------------------------------------//
#include <stdexcept>
#include <utility>
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  get_document_request::get_document_request(std::string index, std::string id)
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

  auto get_document_request::routing(std::string value) -> get_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"Routing value must not be empty"};
    }

    return this->doc_routing = std::move(value), *this;
  }

  auto get_document_request::preference(std::string value) -> get_document_request&
  {
    if (value.empty())
    {
      throw std::invalid_argument{"Preference value must not be empty"};
    }

    return this->doc_preference = std::move(value), *this;
  }

  auto get_document_request::realtime(bool value) noexcept -> get_document_request&
  {
    return this->doc_realtime = value, *this;
  }

  auto get_document_request::refresh(bool value) noexcept -> get_document_request&
  {
    return this->doc_refresh = value, *this;
  }

  auto get_document_request::source(bool value) noexcept -> get_document_request&
  {
    return this->doc_source = value, *this;
  }

  auto get_document_request::source_includes(std::vector<std::string> fields) -> get_document_request&
  {
    return this->doc_source_includes = std::move(fields), *this;
  }

  auto get_document_request::source_excludes(std::vector<std::string> fields) -> get_document_request&
  {
    return this->doc_source_excludes = std::move(fields), *this;
  }

  auto get_document_request::stored_fields(std::vector<std::string> fields) -> get_document_request&
  {
    return this->doc_stored_fields = std::move(fields), *this;
  }

  auto get_document_request::version(std::uint64_t value) noexcept -> get_document_request&
  {
    return this->doc_version = value, *this;
  }

  auto get_document_request::version_type(docapi::version_type value) noexcept -> get_document_request&
  {
    return this->doc_version_type = value, *this;
  }

  auto get_document_request::timeout(std::chrono::milliseconds value) -> get_document_request&
  {
    if (value <= std::chrono::milliseconds::zero())
    {
      throw std::invalid_argument{"Request timeout must be greater than zero"};
    }

    return this->request_timeout = value, *this;
  }

  auto get_document_request::index() const noexcept -> std::string_view
  {
    return this->doc_index;
  }

  auto get_document_request::id() const noexcept -> std::string_view
  {
    return this->doc_id;
  }

  auto get_document_request::build() const -> http::request
  {
    http::request request{http::method_types::get};

    request.endpoint()
           .append(this->doc_index)
           .append("_doc")
           .append(this->doc_id);

    auto& endpoint = request.endpoint();

    if (this->doc_routing.has_value())
    {
        endpoint.query("routing", *this->doc_routing);
    }

    if (this->doc_preference.has_value())
    {
        endpoint.query("preference", *this->doc_preference);
    }

    if (this->doc_realtime.has_value())
    {
        endpoint.query("realtime", bool_value(*this->doc_realtime));
    }

    if (this->doc_refresh.has_value())
    {
        endpoint.query("refresh", bool_value(*this->doc_refresh));
    }

    if (this->doc_source.has_value())
    {
        endpoint.query("_source", bool_value(*this->doc_source));
    }

    if (not this->doc_source_includes.empty())
    {
        endpoint.query("_source_includes", join(this->doc_source_includes));
    }

    if (not this->doc_source_excludes.empty())
    {
        endpoint.query("_source_excludes", join(this->doc_source_excludes));
    }

    if (not this->doc_stored_fields.empty())
    {
        endpoint.query("stored_fields", join(this->doc_stored_fields));
    }

    if (this->doc_version.has_value())
    {
        endpoint.query("version", std::to_string(*this->doc_version));
    }

    if (this->doc_version_type.has_value())
    {
        endpoint.query("version_type", to_string(*this->doc_version_type));
    }

    return request.accept_json().timeout(this->request_timeout), request;
  }

  auto get_document_request::bool_value(bool value) noexcept -> std::string_view
  {
    return value ? "true" : "false";
  }

  auto get_document_request::join(const std::vector<std::string>& values) -> std::string
  {
    std::string result;

    std::size_t required_size = values.empty() ? 0U : values.size() - 1U;
    for (const auto& value : values)
    {
      required_size += value.size();
    }

    result.reserve(required_size);

    for (std::size_t index = 0; index < values.size(); ++index)
    {
      if (index != 0U)
      {
        result.push_back(',');
      }

      result.append(values[index]);
    }

    return result;
  }

  auto get_document_request::to_string(docapi::version_type value) noexcept -> std::string_view
  {
    switch (value)
    {
      case docapi::version_type::internal: return "internal";
      case docapi::version_type::external: return "external";
      case docapi::version_type::external_gte: return "external_gte";
      case docapi::version_type::force: return "force";
    }

    return "internal";
  }
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
