#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include <stdexcept>
#include <string>
#include <type_traits>
//-------------------------------------------------------------------------//
#include "elastic/http/client.h"
//-------------------------------------------------------------------------//
namespace elastic::http::tests
{
//-------------------------------------------------------------------------//
  TEST(HttpClient, NormalizesBaseUrl)
  {
    elastic::http::client client("http://localhost:9200");

    EXPECT_EQ(client.base_url(), "http://localhost:9200");
  }

  TEST(HttpClient, ReplacesBaseUrl)
  {
    elastic::http::client client("http://localhost:9200");
    client.base_url("https://search.example.test:9443");

    EXPECT_EQ(client.base_url(), "https://search.example.test:9443");
  }

  TEST(HttpClient, RejectsEmptyBaseUrl)
  {
    EXPECT_THROW(elastic::http::client(std::string{}), std::invalid_argument);
  }

  TEST(HttpClient, IsNotCopyableOrMovable)
  {
    static_assert(!std::is_copy_constructible_v<elastic::http::client>);
    static_assert(!std::is_copy_assignable_v<elastic::http::client>);
    static_assert(!std::is_move_constructible_v<elastic::http::client>);
    static_assert(!std::is_move_assignable_v<elastic::http::client>);
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::tests
