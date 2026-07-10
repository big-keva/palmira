#include "../simd-json-mget-parser.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include <string>
//-------------------------------------------------------------------------//
#include "../../simd-json-errors.h"
#include "../../simd-json-common.h"
//-------------------------------------------------------------------------//
namespace {
  TEST(SimdJsonDocumentApiParserTest, ValidateIndexDocumentBody) {
    const std::string body =
      R"json({
        "name": "brave",
        "age": 42
      })json";

    EXPECT_NO_THROW(docapi::json::validate_json_object(body));
  }

  TEST(SimdJsonDocumentApiParserTest, RejectEmptyIndexDocumentBody) {
    EXPECT_THROW(docapi::json::validate_json_object(""), docapi::json::parse_error);
  }

  TEST(SimdJsonDocumentApiParserTest, RejectInvalidIndexDocumentBody) {
    const std::string body =
      R"json({
        "name": "brave",
      )json";

    EXPECT_THROW(docapi::json::validate_json_object(body), docapi::json::parse_error);
  }

  TEST(SimdJsonDocumentApiParserTest, RejectArrayIndexDocumentBody) {
    const std::string body =
      R"json([
        {
          "name": "brave"
        }
      ])json";

    EXPECT_THROW(docapi::json::validate_json_object(body), docapi::json::parse_error);
  }

  TEST(SimdJsonDocumentApiParserTest, ParseMgetDocsForm) {
    const std::string body =
      R"json({
        "docs": [
          {
            "_index": "test",
            "_id": "1"
          },
          {
            "_index": "test",
            "_id": "2"
          }
        ]
      })json";

    auto request = docapi::json::elastic::parse_mget_request(body, "");
    ASSERT_EQ(request.docs.size(), 2U);

    EXPECT_EQ(request.docs[0].index, "test");
    EXPECT_EQ(request.docs[0].id, "1");

    EXPECT_EQ(request.docs[1].index, "test");
    EXPECT_EQ(request.docs[1].id, "2");
  }

  TEST(SimdJsonDocumentApiParserTest, ParseMgetIdsFormWithDefaultIndex) {
    const std::string body =
      R"json({
        "ids": ["1", "2", "3"]
      })json";

    auto request = docapi::json::elastic::parse_mget_request(body, "test");

    ASSERT_EQ(request.docs.size(), 3U);

    EXPECT_EQ(request.docs[0].index, "test");
    EXPECT_EQ(request.docs[0].id, "1");

    EXPECT_EQ(request.docs[1].index, "test");
    EXPECT_EQ(request.docs[1].id, "2");

    EXPECT_EQ(request.docs[2].index, "test");
    EXPECT_EQ(request.docs[2].id, "3");
  }

  TEST(SimdJsonDocumentApiParserTest, RejectMgetIdsFormWithoutDefaultIndex) {
    const std::string body =
      R"json({
        "ids": ["1", "2"]
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_mget_request(body, ""), docapi::json::parse_error);
  }

  TEST(SimdJsonDocumentApiParserTest, ParseMgetSourceFiltering) {
    const std::string body =
      R"json({
        "docs": [
          {
            "_index": "test",
            "_id": "1",
            "_source": {
              "includes": ["name", "title"],
              "excludes": ["blob"]
            }
          }
        ]
      })json";

    auto request = docapi::json::elastic::parse_mget_request(body, "");
    ASSERT_EQ(request.docs.size(), 1U);

    const auto &doc = request.docs[0];

    EXPECT_EQ(doc.index, "test");
    EXPECT_EQ(doc.id, "1");

    EXPECT_TRUE(doc.source.enabled);

    ASSERT_EQ(doc.source.includes.size(), 2U);
    EXPECT_EQ(doc.source.includes[0], "name");
    EXPECT_EQ(doc.source.includes[1], "title");

    ASSERT_EQ(doc.source.excludes.size(), 1U);
    EXPECT_EQ(doc.source.excludes[0], "blob");
  }

  TEST(SimdJsonDocumentApiParserTest, ParseMgetSourceDisabled) {
    const std::string body =
      R"json({
        "docs": [
          {
            "_index": "test",
            "_id": "1",
            "_source": false
          }
        ]
      })json";

    auto request = docapi::json::elastic::parse_mget_request(body, "");
    ASSERT_EQ(request.docs.size(), 1U);

    EXPECT_FALSE(request.docs[0].source.enabled);
  }

  TEST(SimdJsonDocumentApiParserTest, RejectMgetDocWithoutIndex) {
    const std::string body =
      R"json({
        "docs": [
          {
            "_id": "1"
          }
        ]
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_mget_request(body, ""), docapi::json::parse_error);
  }

  TEST(SimdJsonDocumentApiParserTest, RejectMgetDocWithoutId) {
    const std::string body =
      R"json({
        "docs": [
          {
            "_index": "test"
          }
        ]
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_mget_request(body, ""), docapi::json::parse_error);
  }
//-------------------------------------------------------------------------//
} // namespace
