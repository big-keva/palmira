#pragma once
//-------------------------------------------------------------------------//
#include <string>
#include <utility>
//-------------------------------------------------------------------------//
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
#if __cplusplus >= 202002L
    #include <concepts>

    template<class request_t>
    concept rest_request = requires(const request_t &request, http::response response) {
      typename request_t::response_type;
      { request.build() } -> std::same_as<http::request>;
      { typename request_t::response_type{std::move(response)} };
    };

    #define REST_REQUEST_CONCEPT 1
    #define REST_REQUEST_CHECK(T) (rest_request<T>)
#else
#error Adding concepts checking for C++17 support.
#endif
//-------------------------------------------------------------------------//
  } // namespace detail
//-------------------------------------------------------------------------//
  class client
  {
    //!< Keeps HTTP client.
    http::client http_client;

  public:
    client(const client&) = delete;
    auto operator=(const client &) -> client & = delete;

    client(client&&) = delete;
    auto operator=(client &&) -> client & = delete;

  public:
    /**
     * Constructor.
     * @param base_url [in] - A base url.
     */
    explicit client(std::string base_url);

    /**
     * Destructor.
     */
    ~client() = default;

    /**
     * Sets a base url.
     * @param value [in] - A base url.
     * @return The reference on the instance.
     */
    auto base_url(std::string value) -> client &;

    /**
     * Gets a base url.
     * @return A base url.
     */
    [[nodiscard]] auto base_url() const -> std::string;

    /**
     * Executes a request.
     * @param request [in] - A request.
     * @return A response.
     */
    [[nodiscard]] auto execute(const http::request& request) -> http::response;

    template<detail::rest_request request_t>
    [[nodiscard]] auto execute(const request_t &request) -> typename request_t::response_type
    {
      auto http_request = request.build();
      auto http_response = this->http_client.execute(http_request);

      return typename request_t::response_type{std::move(http_response)};
    }
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api
