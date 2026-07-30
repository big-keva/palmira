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

    [[nodiscard]]
    auto status() const noexcept -> std::uint16_t
    {
      return this->resp.get_status();
    }

    [[nodiscard]]
    auto body() const noexcept -> std::string_view
    {
      return this->resp.body();
    }
  };

  class test_request
  {
  public:
      using response_type = test_response;

      [[nodiscard]]
      auto build() const -> elastic::http::request
      {
          elastic::http::request request{elastic::http::method_types::get};
          request.endpoint().append("_cluster").append("health");
          return request;
      }
  };

  static_assert(elastic::api::detail::rest_request<test_request>);
  static_assert(std::is_same_v<decltype(std::declval<const test_request&>().build()), elastic::http::request>);
  static_assert(std::is_constructible_v<test_response, elastic::http::response>);
  static_assert(!std::is_copy_constructible_v<elastic::api::client>);
  static_assert(!std::is_move_constructible_v<elastic::api::client>);

  TEST(ApiClient, StoresBaseUrl)
  {
    elastic::api::client client{"http://localhost:9200/"};

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

TEST(ApiClient, BuildsFluentPostRequest)
{
  elastic::api::client client{"http://localhost:9200"};

  auto builder = client.method(elastic::http::method_types::post);

  builder
      .path("books/_doc/42")
      .timeout(10000)
      .body(R"({"name":"brave"})")
      .query("refresh", "wait_for")
      .accept_json()
      .content_type_json();

  EXPECT_EQ(builder.request().method(), elastic::http::method_types::post);
  EXPECT_EQ(builder.request().target(), "/books/_doc/42?refresh=wait_for");
  EXPECT_EQ(builder.request().timeout(), std::chrono::milliseconds{10000});
  EXPECT_EQ(builder.request().body(), R"({"name":"brave"})");
  EXPECT_TRUE(builder.request().headers().contains("Accept"));
  EXPECT_TRUE(builder.request().headers().contains("Content-Type"));
}

TEST(ApiClient, ConvenienceMethodBuildsGetRequest)
{
  elastic::api::client client{"http://localhost:9200"};

  auto builder = client.get("books/_doc/42");

  builder
      .query("realtime", true)
      .query("version", std::int64_t{7});

  EXPECT_EQ(builder.request().method(), elastic::http::method_types::get);
  EXPECT_EQ(builder.request().target(), "/books/_doc/42?realtime=true&version=7");
}


TEST(ApiClient, BuildsTypedIndexDocumentWithGeneratedId)
{
  elastic::api::client client{"http://localhost:9200"};

  auto builder = client.index("books");
  builder.body(R"({"name":"brave"})")
         .query("refresh", "wait_for")
         .timeout(10000);

  EXPECT_EQ(builder.request().method(), elastic::http::method_types::post);
  EXPECT_EQ(builder.request().target(), "/books/_doc?refresh=wait_for");
  EXPECT_EQ(builder.request().body(), R"({"name":"brave"})");
  EXPECT_EQ(builder.request().timeout(), std::chrono::milliseconds{10000});
  EXPECT_TRUE(builder.request().headers().contains("Accept"));
  EXPECT_TRUE(builder.request().headers().contains("Content-Type"));
}

TEST(ApiClient, BuildsTypedIndexDocumentWithExplicitId)
{
  elastic::api::client client{"http://localhost:9200"};

  auto builder = client.index("books", "doc/42");
  builder.body(R"({"name":"brave"})");

  EXPECT_EQ(builder.request().method(), elastic::http::method_types::put);
  EXPECT_EQ(builder.request().target(), "/books/_doc/doc%2F42");
}

TEST(ApiClient, BuildsTypedGetDocument)
{
  elastic::api::client client{"http://localhost:9200"};

  auto builder = client.get_document("books", "42");
  builder.query("realtime", true)
         .timeout(5000);

  EXPECT_EQ(builder.request().method(), elastic::http::method_types::get);
  EXPECT_EQ(builder.request().target(), "/books/_doc/42?realtime=true");
  EXPECT_EQ(builder.request().timeout(), std::chrono::milliseconds{5000});
  EXPECT_TRUE(builder.request().headers().contains("Accept"));
  EXPECT_FALSE(builder.request().headers().contains("Content-Type"));
}

TEST(ApiClient, BuildsTypedDeleteDocument)
{
  elastic::api::client client{"http://localhost:9200"};

  auto builder = client.delete_document("books", "42");
  builder.query("refresh", "wait_for");

  EXPECT_EQ(builder.request().method(), elastic::http::method_types::del);
  EXPECT_EQ(builder.request().target(), "/books/_doc/42?refresh=wait_for");
  EXPECT_TRUE(builder.request().headers().contains("Accept"));
}

TEST(ApiClient, RejectsEmptyDocumentPathSegments)
{
  elastic::api::client client{"http://localhost:9200"};

  EXPECT_THROW(client.index(""), std::invalid_argument);
  EXPECT_THROW(client.index("books", ""), std::invalid_argument);
  EXPECT_THROW(client.get_document("", "42"), std::invalid_argument);
  EXPECT_THROW(client.delete_document("books", ""), std::invalid_argument);
}
