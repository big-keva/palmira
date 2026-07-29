#include "elastic/http/session.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
namespace elastic::http::tests
{
//-------------------------------------------------------------------------//
  TEST(SessionTest, NormalizesTrailingSlashesInBaseUrl)
  {
      session value{"http://localhost:9200"};

      EXPECT_EQ(value.base_url(), "http://localhost:9200");
  }

  TEST(SessionTest, AllowsChangingBaseUrl)
  {
      session value;

      value.base_url("https://example.test:9443/base/");

      EXPECT_EQ(value.base_url(), "https://example.test:9443/base");
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::tests
