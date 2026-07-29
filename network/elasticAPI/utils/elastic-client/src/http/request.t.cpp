#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include <chrono>
#include <string>
//-------------------------------------------------------------------------//
#include "elastic/http/request.h"
//-------------------------------------------------------------------------//
namespace elastic::http::tests
{
//-------------------------------------------------------------------------//
  using namespace std::chrono_literals;
//-------------------------------------------------------------------------//
  TEST(HttpRequest, UsesSafeDefaults)
  {
    const elastic::http::request request;

    EXPECT_EQ(request.method(), elastic::http::method_types::get);
    EXPECT_EQ(request.target(), "/");
    EXPECT_FALSE(request.has_body());
    EXPECT_EQ(request.timeout(), 30s);
    EXPECT_TRUE(request.headers().empty());
  }

  TEST(HttpRequest, BuildsRequest)
  {
    elastic::http::request request;

    request.method(elastic::http::method_types::put)
           .endpoint().append("books").append("_doc").append("42");

    request.body(std::string_view(R"({"title":"C++"})"))
           .accept_json()
           .content_type_json()
           .timeout(5s);

    EXPECT_EQ(request.method(), elastic::http::method_types::put);
    EXPECT_EQ(request.target(), "/books/_doc/42");
    EXPECT_EQ(request.body(), R"({"title":"C++"})");
    EXPECT_EQ(request.headers().get("accept"), "application/json");
    EXPECT_EQ(request.headers().get("CONTENT-TYPE"), "application/json");
    EXPECT_EQ(request.timeout(), 5s);
  }

  TEST(HttpRequest, OwnsHeaderStrings)
  {
    elastic::http::request request;

    {
      std::string name = "X-Request-Id";
      std::string value = "request-42";
      request.header(name, value);
    }

    EXPECT_EQ(request.headers().get("x-request-id"), "request-42");
  }

  TEST(HttpRequest, SetHeaderReplacesAllExistingValues)
  {
    elastic::http::request request;

    request.header("Accept", "text/plain")
           .header("accept", "application/xml")
           .set_header("ACCEPT", "application/json");

    EXPECT_EQ(request.headers().size(), 1U);
    EXPECT_EQ(request.headers().get("Accept"), "application/json");
  }

  TEST(HttpRequest, RejectsInvalidTimeout)
  {
    elastic::http::request request;

    EXPECT_THROW(request.timeout(0ms), std::invalid_argument);
    EXPECT_THROW(request.timeout(-1ms), std::invalid_argument);
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::tests
