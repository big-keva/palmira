#include "../simd-json-search-parser.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
//-------------------------------------------------------------------------//
#include "../../simd-json-errors.h"
//-------------------------------------------------------------------------//
namespace {
//-------------------------------------------------------------------------//
  TEST(SimdJsonParserSearchTest, ParseSimpleMatchQuery) {
    const std::string body = R"json({"query": {"match": {"name": "brave"}}})json";
    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    ASSERT_EQ(request.indices.size(), 1U);
    EXPECT_EQ(request.indices[0], "test");

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match);

    const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value()->data);

    EXPECT_EQ(match.field, "name");
    EXPECT_EQ(match.query, "brave");

    EXPECT_TRUE(match.operator_value.empty());
    EXPECT_TRUE(match.analyzer.empty());
    EXPECT_TRUE(match.fuzziness.empty());
    EXPECT_TRUE(match.minimum_should_match.empty());
    EXPECT_TRUE(match.zero_terms_query.empty());

    EXPECT_FALSE(match.lenient);
    EXPECT_TRUE(match.auto_generate_synonyms_phrase_query);
    EXPECT_DOUBLE_EQ(match.boost, 1.0);
  }

  TEST(SimdJsonParserSearchTest, ParseFullMatchQuery) {
    const std::string body =
      R"json({
        "from": 0,
        "size": 25,
        "query": {
          "match": {
            "name": {
              "query": "brave",
              "operator": "and",
              "analyzer": "standard",
              "fuzziness": "AUTO",
              "minimum_should_match": "75%",
              "zero_terms_query": "none",
              "lenient": true,
              "auto_generate_synonyms_phrase_query": false,
              "boost": 1.5
            }
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    EXPECT_EQ(request.from, 0U);
    EXPECT_EQ(request.size, 25U);

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match);

    const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value()->data);

    EXPECT_EQ(match.field, "name");
    EXPECT_EQ(match.query, "brave");
    EXPECT_EQ(match.operator_value, "and");
    EXPECT_EQ(match.analyzer, "standard");
    EXPECT_EQ(match.fuzziness, "AUTO");
    EXPECT_EQ(match.minimum_should_match, "75%");
    EXPECT_EQ(match.zero_terms_query, "none");

    EXPECT_TRUE(match.lenient);
    EXPECT_FALSE(match.auto_generate_synonyms_phrase_query);
    EXPECT_DOUBLE_EQ(match.boost, 1.5);
  }

  TEST(SimdJsonParserSearchTest, ParseMatchQueryWithScalarNumericValue) {
    const std::string body =
      R"json({
        "query": {
          "match": {
            "age": 42
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"users"});

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match);

    const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value()->data);

    EXPECT_EQ(match.field, "age");
    EXPECT_EQ(match.query, "42");
  }

  TEST(SimdJsonParserSearchTest, ParseMatchQueryWithScalarBooleanValue) {
    const std::string body =
      R"json({
        "query": {
          "match": {
            "active": true
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"users"});

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match);

    const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value()->data);

    EXPECT_EQ(match.field, "active");
    EXPECT_EQ(match.query, "true");
  }

  TEST(SimdJsonParserSearchTest, RejectInvalidMatchOperator) {
    const std::string body =
      R"json({
        "query": {
          "match": {
            "name": {
              "query": "brave",
              "operator": "xor"
            }
          }
        }
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_search_request(body, {"test"}), docapi::json::parse_error);
  }

  TEST(SimdJsonParserSearchTest, RejectInvalidZeroTermsQuery) {
    const std::string body =
      R"json({
        "query": {
          "match": {
            "name": {
              "query": "brave",
              "zero_terms_query": "invalid"
            }
          }
        }
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_search_request(body, {"test"}), docapi::json::parse_error);
  }

  TEST(SimdJsonParserSearchTest, RejectEmptyMatchObject) {
    const std::string body =
      R"json({
        "query": {
          "match": {}
        }
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_search_request(body, {"test"}), docapi::json::parse_error);
  }

  TEST(SimdJsonParserSearchTest, DetectMatchAll) {
    const std::string body =
      R"json({
        "query": {
          "match_all": {}
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match_all);

    //<TODO> const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value().data);
  }

  TEST(SimdJsonParserSearchTest, DetectMatchNone) {
    const std::string body =
      R"json({
        "query": {
          "match_none": {
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match_none);

    //<TODO> const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value().data);
  }

  TEST(SimdJsonParserSearchTest, ParseSearchRequestExtendedFields) {
    const std::string body =
      R"json({
        "query": {
          "match": {
            "name": "brave"
          }
        },
        "from": 0,
        "size": 25,
        "_source": {
          "includes": ["name", "title"],
          "excludes": ["blob"]
        },
        "sort": [
          {
            "created_at": {
              "order": "desc"
            }
          },
          "_score"
        ],
        "stored_fields": ["_none_"],
        "fields": ["name", "title"],
        "docvalue_fields": [
          {
            "field": "created_at",
            "format": "epoch_millis"
          }
        ],
        "search_after": [100, "abc"],
        "track_total_hits": 10000,
        "track_scores": true,
        "version": true,
        "seq_no_primary_term": true,
        "terminate_after": 5000,
        "min_score": 0.5
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    ASSERT_EQ(request.indices.size(), 1U);
    EXPECT_EQ(request.indices[0], "test");

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::match);

    const auto match = std::get<docapi::json::elastic::search::match_query>(request.query.value()->data);

    EXPECT_EQ(match.field, "name");
    EXPECT_EQ(match.query, "brave");

    EXPECT_EQ(request.from, 0U);
    EXPECT_EQ(request.size, 25U);

    EXPECT_TRUE(request.source.enabled);

    ASSERT_EQ(request.source.includes.size(), 2U);
    EXPECT_EQ(request.source.includes[0], "name");
    EXPECT_EQ(request.source.includes[1], "title");

    ASSERT_EQ(request.source.excludes.size(), 1U);
    EXPECT_EQ(request.source.excludes[0], "blob");

    ASSERT_EQ(request.sort.size(), 2U);
    EXPECT_EQ(request.sort[0].field, "created_at");
    EXPECT_EQ(request.sort[0].order, "desc");
    EXPECT_EQ(request.sort[1].field, "_score");
    EXPECT_EQ(request.sort[1].order, "asc");

    ASSERT_EQ(request.stored_fields.size(), 1U);
    EXPECT_EQ(request.stored_fields[0], "_none_");

    ASSERT_EQ(request.fields.size(), 2U);
    EXPECT_EQ(request.fields[0], "name");
    EXPECT_EQ(request.fields[1], "title");

    ASSERT_EQ(request.docvalue_fields.size(), 1U);
    EXPECT_EQ(request.docvalue_fields[0].field, "created_at");
    EXPECT_EQ(request.docvalue_fields[0].format, "epoch_millis");

    ASSERT_EQ(request.search_after.size(), 2U);
    EXPECT_EQ(request.search_after[0], "100");
    EXPECT_EQ(request.search_after[1], "abc");

    EXPECT_TRUE(request.track_total_hits_enabled);
    ASSERT_TRUE(request.track_total_hits_limit.has_value());
    EXPECT_EQ(*request.track_total_hits_limit, 10000U);

    EXPECT_TRUE(request.track_scores);
    EXPECT_TRUE(request.version);
    EXPECT_TRUE(request.seq_no_primary_term);

    EXPECT_EQ(request.terminate_after, 5000U);

    EXPECT_TRUE(request.has_min_score);
    EXPECT_DOUBLE_EQ(request.min_score, 0.5);
  }

  TEST(SimdJsonParserSearchTest, RejectInvalidSortOrder) {
    const std::string body =
      R"json({
        "sort": [
          {
            "created_at": {
              "order": "sideways"
            }
          }
        ]
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_search_request(body, {"test"}), docapi::json::parse_error);
  }

  TEST(SimdJsonParserSearchTest, ParseTrackTotalHitsFalse) {
    const std::string body =
      R"json({
        "track_total_hits": false
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    EXPECT_FALSE(request.track_total_hits_enabled);
    EXPECT_FALSE(request.track_total_hits_limit.has_value());
  }
//-------------------------------------------------------------------------//
  TEST(SimdJsonParserSearchTest, ParseSimpleTermQuery) {
    const std::string body =
      R"json({
        "query": {
          "term": {
            "name": "brave"
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});

    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::term);

    const auto term = std::get<docapi::json::elastic::search::term_query>(request.query.value()->data);

    EXPECT_EQ(term.field, "name");
    EXPECT_EQ(term.value, "brave");
    EXPECT_DOUBLE_EQ(term.boost, 1.0);
  }

  TEST(SimdJsonParserSearchTest, ParseExtendedTermQuery) {
    const std::string body =
      R"json({
        "query": {
          "term": {
            "name": {
              "value": "brave",
              "boost": 2.5
            }
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});
    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::term);

    const auto term = std::get<docapi::json::elastic::search::term_query>(request.query.value()->data);

    EXPECT_EQ(term.field, "name");
    EXPECT_EQ(term.value, "brave");
    EXPECT_DOUBLE_EQ(term.boost, 2.5);
  }

  TEST(SimdJsonParserSearchTest, RejectTermWithoutValue) {
    const std::string body =
      R"json({
        "query": {
          "term": {
            "name": {
              "boost": 5.0
            }
          }
        }
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_search_request(body, {"test"}), docapi::json::parse_error);
  }

  TEST(SimdJsonParserSearchTest, ParseTermScalarValue) {
    const std::string body =
      R"json({
        "query": {
          "term": {
            "age": 42
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});
    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::term);

    const auto term = std::get<docapi::json::elastic::search::term_query>(request.query.value()->data);

    EXPECT_EQ(term.field, "age");
    EXPECT_EQ(term.value, "42");
  }

  TEST(SimdJsonParserSearchTest, ParseTermsQuery) {
    const std::string body =
      R"json({
        "query": {
          "terms": {
            "status": ["new", "active", "blocked"]
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});
    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::terms);

    const auto terms = std::get<docapi::json::elastic::search::terms_query>(request.query.value()->data);

    EXPECT_EQ(terms.field, "status");
    ASSERT_EQ(terms.values.size(), 3U);
    EXPECT_EQ(terms.values[0], "new");
    EXPECT_EQ(terms.values[1], "active");
    EXPECT_EQ(terms.values[2], "blocked");
  }
//-------------------------------------------------------------------------//
  TEST(SimdJsonParserSearchTest, ParseRangeQuery) {
    const std::string body =
      R"json({
        "query": {
          "range": {
            "age": {
              "gte": 18,
              "lt": 65,
              "boost": 2.0
            }
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"users"});
    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::range);

    const auto range = std::get<docapi::json::elastic::search::range_query>(request.query.value()->data);

    EXPECT_EQ(range.field, "age");
    ASSERT_TRUE(range.gte.has_value());
    ASSERT_TRUE(range.lt.has_value());

    EXPECT_EQ(*range.gte, "18");
    EXPECT_EQ(*range.lt, "65");
    EXPECT_DOUBLE_EQ(range.boost, 2.0);
  }

  TEST(SimdJsonParserSearchTest, RejectRangeWithoutBounds) {
    const std::string body =
      R"json({
        "query": {
          "range": {
            "age": {
              "boost": 2.0
            }
          }
        }
      })json";

    EXPECT_THROW(docapi::json::elastic::parse_search_request(body, {"users"}), docapi::json::parse_error);
  }
//-------------------------------------------------------------------------//
  TEST(SimdJsonParserSearchTest, ParseBoolQuery) {
    const std::string body =
      R"json({
        "query": {
          "bool": {
            "must": [
              {
                "match": {
                  "title": "linux"
                }
              }
            ],
            "filter": [
              {
                "term": {
                  "status": "active"
                }
              }
            ],
            "must_not": {
              "term": {
                "deleted": true
              }
            },
            "minimum_should_match": 1,
            "boost": 2.0
          }
        }
      })json";

    auto request = docapi::json::elastic::parse_search_request(body, {"test"});
    ASSERT_TRUE(request.query.has_value());
    EXPECT_EQ(request.query.value()->kind, docapi::json::query_kinds::bool_query);

    const auto bool_query = std::get<docapi::json::elastic::search::bool_query_t>(request.query.value()->data);

    ASSERT_NE(bool_query, nullptr);

    ASSERT_EQ(bool_query->must.size(), 1U);
    ASSERT_EQ(bool_query->filter.size(), 1U);
    ASSERT_EQ(bool_query->must_not.size(), 1U);

    EXPECT_EQ(bool_query->minimum_should_match, 1U);
    EXPECT_DOUBLE_EQ(bool_query->boost, 2.0);

    EXPECT_EQ(bool_query->must[0]->kind, docapi::json::query_kinds::match);
    EXPECT_EQ(bool_query->filter[0]->kind, docapi::json::query_kinds::term);
    EXPECT_EQ(bool_query->must_not[0]->kind, docapi::json::query_kinds::term);
  }
//-------------------------------------------------------------------------//
} // namespace
