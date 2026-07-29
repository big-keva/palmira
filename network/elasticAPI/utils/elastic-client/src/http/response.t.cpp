#include "elastic/http/response.h"
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
namespace elastic::http::detail
{
//-------------------------------------------------------------------------//
  struct response_test_access
  {
    static auto assign(response &value, long status, std::string_view headers, std::string_view body = {}) -> void
    {
      value.clear();
      value.response_status = status;
      value.raw_headers.assign(headers.begin(), headers.end());
      value.raw_body.assign(body.begin(), body.end());
    }
  };
//-------------------------------------------------------------------------//
} // namespace elastic::http::detail
//-------------------------------------------------------------------------//
namespace elastic::http::tests
{
//-------------------------------------------------------------------------//
using elastic::http::response;
using elastic::http::detail::response_test_access;
//-------------------------------------------------------------------------//
  TEST(response_test, keeps_only_last_http_header_block)
  {
    response value;

    response_test_access::assign(
        value, 200L,
        "HTTP/1.1 100 Continue\r\n"
        "X-Interim: ignored\r\n"
        "\r\n"
        "HTTP/1.1 302 Found\r\n"
        "Location: /final\r\n"
        "X-Redirect: ignored\r\n"
        "\r\n"
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "X-Final: yes\r\n"
        "\r\n");

    EXPECT_FALSE(value.has_header("X-Interim"));
    EXPECT_FALSE(value.has_header("Location"));
    EXPECT_FALSE(value.has_header("X-Redirect"));
    EXPECT_EQ(value.header("X-Final"), "yes");
    EXPECT_TRUE(value.is_json());
  }

  TEST(response_test, recognizes_json_suffix_and_chunked_token)
  {
    response value;

    response_test_access::assign(
        value, 200L,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/vnd.opensearch+json\r\n"
        "Transfer-Encoding: gzip, Chunked\r\n"
        "Connection: keep-alive\r\n"
        "\r\n");

    EXPECT_TRUE(value.is_json());
    EXPECT_TRUE(value.is_chunked());
    EXPECT_TRUE(value.keep_alive());
  }

  TEST(response_test, classifies_status_code)
  {
    response value;

    response_test_access::assign(value, 404L, "HTTP/1.1 404 Not Found\r\n\r\n");

    EXPECT_FALSE(value.ok());
    EXPECT_FALSE(value.redirect());
    EXPECT_TRUE(value.client_error());
    EXPECT_FALSE(value.server_error());
    EXPECT_EQ(value.get_status(), 404U);
  }

  TEST(response_test, connection_close_disables_keep_alive)
  {
    response value;

    response_test_access::assign(value, 200L,
                                 "HTTP/1.1 200 OK\r\n"
                                 "Connection: upgrade, close\r\n"
                                 "\r\n");

    EXPECT_FALSE(value.keep_alive());
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::tests
