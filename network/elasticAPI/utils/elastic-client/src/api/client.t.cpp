#include <string_view>
#include <type_traits>
#include <utility>
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include "elastic/api/client.h"
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  class test_response
  {
    elastic::http::response resp;

  public:
    explicit test_response(elastic::http::response response) noexcept
      : resp(std::move(response))
    {
    }

    test_response(const test_response&) = delete;
    auto operator=(const test_response&) -> test_response& = delete;

    test_response(test_response&&) noexcept = default;
    auto operator=(test_response&&) noexcept -> test_response& = default;

    [[nodiscard]] auto status() const noexcept -> std::uint16_t
    {
        return this->resp.get_status();
    }

    [[nodiscard]] auto body() const noexcept -> std::string_view
    {
        return this->resp.body();
    }
  };

  struct test_request
  {
    using response_type = test_response;

    [[nodiscard]] auto build() const -> elastic::http::request
    {
      elastic::http::request request{elastic::http::method_types::get};
      request.endpoint().append("_cluster").append("health");
      return request;
    }
  };
//-------------------------------------------------------------------------//
  static_assert(elastic::api::detail::rest_request<test_request>);
  static_assert(std::is_same_v<decltype(std::declval<const test_request&>().build()), elastic::http::request>);
  static_assert(std::is_constructible_v<test_response, elastic::http::response>);
  static_assert(!std::is_copy_constructible_v<elastic::api::client>);
  static_assert(!std::is_move_constructible_v<elastic::api::client>);
//-------------------------------------------------------------------------//
  TEST(ApiClient, StoresBaseUrl)
  {
      elastic::api::client client{"http://localhost:9200"};

      EXPECT_EQ(client.base_url(), "http://localhost:9200");
  }

  TEST(ApiClient, ReplacesBaseUrl)
  {
      elastic::api::client client{"http://localhost:9200"};

      client.base_url("http://127.0.0.1:9201/");

      EXPECT_EQ(client.base_url(), "http://127.0.0.1:9201");
  }
//-------------------------------------------------------------------------//
} // namespace
