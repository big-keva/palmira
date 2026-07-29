#include "elastic/http/endpoint.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include <stdexcept>
//-------------------------------------------------------------------------//
namespace elastic::http::tests
{
  TEST(Endpoint, BuildsRootPath)
  {
      endpoint value;
      EXPECT_EQ(value.build(), "/");
  }

  TEST(Endpoint, BuildsPathAndQuery)
  {
      endpoint value;

      value.append("books")
           .append("_doc")
           .append("42")
           .query("refresh", "wait_for")
           .query("routing", "node-1");

      EXPECT_EQ(
          value.build(),
          "/books/_doc/42?refresh=wait_for&routing=node-1");
  }

  TEST(Endpoint, SplitsCompositePath)
  {
      endpoint value;

      value.path("/books//_search/");

      EXPECT_EQ(value.segment_count(), 2U);
      EXPECT_EQ(value.build(), "/books/_search");
  }

  TEST(Endpoint, EncodesPathSegmentsAndQueryParameters)
  {
      endpoint value;

      value.append("index name")
           .append("a/b")
           .query("filter name", "status:active/value");

      EXPECT_EQ(
          value.build(),
          "/index%20name/a%2Fb?filter%20name=status%3Aactive%2Fvalue");
  }

  TEST(Endpoint, KeepsDuplicateQueryParameters)
  {
      endpoint value;

      value.append("_search")
           .query("filter_path", "hits.total")
           .query("filter_path", "took");

      EXPECT_EQ(
          value.build(),
          "/_search?filter_path=hits.total&filter_path=took");
  }

  TEST(Endpoint, ReplacesAndRemovesLastSegment)
  {
      endpoint value;

      value.append("books")
           .append("_doc")
           .append("old-id")
           .replace_last("new-id");

      EXPECT_EQ(value.build(), "/books/_doc/new-id");

      value.remove_last();
      EXPECT_EQ(value.build(), "/books/_doc");
  }

  TEST(Endpoint, QueryIfAddsOnlyEnabledParameter)
  {
      endpoint value;

      value.append("_search")
           .query_if(false, "pretty", "true")
           .query_if(true, "typed_keys", "true");

      EXPECT_EQ(value.build(), "/_search?typed_keys=true");
  }

  TEST(Endpoint, ClearRemovesAllState)
  {
      endpoint value;

      value.append("books")
           .query("pretty", "true")
           .clear();

      EXPECT_TRUE(value.empty());
      EXPECT_EQ(value.build(), "/");
  }

  TEST(Endpoint, RejectsEmptySegmentAndQueryName)
  {
      endpoint value;

      EXPECT_THROW(value.append(""), std::invalid_argument);
      EXPECT_THROW(value.query("", "value"), std::invalid_argument);
      EXPECT_THROW(value.replace_last("segment"), std::logic_error);
  }
//-------------------------------------------------------------------------//
} // namespace elastic::http::tests
