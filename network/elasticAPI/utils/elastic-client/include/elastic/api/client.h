#pragma once
//-------------------------------------------------------------------------//
#include <concepts>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
//-------------------------------------------------------------------------//
#include "elastic/api/request_builder.h"
#include "elastic/api/typed_request_builder.h"
#include "elastic/api/docapi/delete_document_response.h"
#include "elastic/api/docapi/get_document_response.h"
#include "elastic/api/docapi/index_document_response.h"
#include "elastic/http/client.h"
#include "elastic/http/request.h"
#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
namespace elastic::api
{
//-------------------------------------------------------------------------//
  namespace detail
  {
//-------------------------------------------------------------------------//
    template<class Request>
    concept rest_request = requires(const Request& request, http::response response)
    {
      typename Request::response_type;
      {
        request.build()
      } -> std::same_as<http::request>;
      {
        typename Request::response_type{std::move(response)}
      };
    };
//-------------------------------------------------------------------------//
  } // namespace detail
//-------------------------------------------------------------------------//
  class client
  {
    //!< Keeps HTTP client.
    http::client http;

  public:
    /**
     * Constructor.
     * @param base_url
     */
    explicit client(std::string base_url);

    client(const client&) = delete;
    auto operator=(const client&) -> client& = delete;

    client(client&&) = delete;
    auto operator=(client&&) -> client& = delete;

    /**
     * Destructor.
     */
    ~client() = default;

    template<detail::rest_request Request>
    [[nodiscard]]
    auto execute(const Request& request) -> typename Request::response_type
    {
      auto http_request = request.build();
      auto http_response = this->http.execute(http_request);

      return typename Request::response_type{std::move(http_response)};
    }

    [[nodiscard]]
    auto method(http::method_types request_method) -> request_builder;

    [[nodiscard]]
    auto get(std::string_view path = {}) -> request_builder;

    [[nodiscard]]
    auto post(std::string_view path = {}) -> request_builder;

    [[nodiscard]]
    auto put(std::string_view path = {}) -> request_builder;

    [[nodiscard]]
    auto del(std::string_view path = {}) -> request_builder;

    [[nodiscard]]
    auto index(std::string_view index_name) -> typed_request_builder<docapi::index_document_response>;

    [[nodiscard]]
    auto index(std::string_view index_name, std::string_view document_id) -> typed_request_builder<docapi::index_document_response>;

    [[nodiscard]]
    auto get_document(std::string_view index_name, std::string_view document_id) -> typed_request_builder<docapi::get_document_response>;

    [[nodiscard]]
    auto delete_document(std::string_view index_name, std::string_view document_id) -> typed_request_builder<docapi::delete_document_response>;

    [[nodiscard]]
    auto execute(const http::request& request) -> http::response;

    auto base_url(std::string value) -> client &;

    [[nodiscard]]
    auto base_url() const -> std::string;

  private:
    static auto validate_path_segment(std::string_view value, const char* message) -> void
    {
      if (value.empty())
      {
        throw std::invalid_argument{message};
      }
    }
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api
