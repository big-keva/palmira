#include <chrono>
#include <stdexcept>
#include <type_traits>
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include "elastic/api/client.h"
#include "elastic/api/docapi/index_document_request.h"
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  using elastic::api::docapi::index_document_request;
  using elastic::api::docapi::index_document_response;
  using elastic::api::docapi::index_operation;
  using elastic::api::docapi::refresh_policies;
  using elastic::api::docapi::version_types;
//-------------------------------------------------------------------------//
  static_assert(elastic::api::detail::rest_request<index_document_request>);
  static_assert(std::is_same_v<index_document_request::response_type, index_document_response>);
  static_assert(!std::is_copy_constructible_v<index_document_response>);
  static_assert(std::is_move_constructible_v<index_document_response>);
//-------------------------------------------------------------------------//
  TEST(IndexDocumentRequest, BuildsPostWithGeneratedId)
  {
      const index_document_request api_request{"books", R"({"title":"C++"})"};
      const auto request = api_request.build();

      EXPECT_EQ(request.method(), elastic::http::method_types::post);
      EXPECT_EQ(request.target(), "/books/_doc");
      EXPECT_EQ(request.body(), R"({"title":"C++"})");
      EXPECT_EQ(request.timeout(), std::chrono::seconds{30});
      EXPECT_TRUE(request.headers().contains("Accept"));
      EXPECT_TRUE(request.headers().contains("Content-Type"));
  }

  TEST(IndexDocumentRequest, BuildsPutWithExplicitId)
  {
      const index_document_request api_request{
          "books archive",
          "part/42",
          R"({"title":"C++"})"};

      const auto request = api_request.build();

      EXPECT_EQ(request.method(), elastic::http::method_types::put);
      EXPECT_EQ(request.target(), "/books%20archive/_doc/part%2F42");
  }

  TEST(IndexDocumentRequest, BuildsQueryParameters)
  {
      index_document_request api_request{"books", "42", R"({"title":"C++"})"};

      api_request.routing("tenant/7")
                 .pipeline("normalize")
                 .operation(index_operation::create)
                 .refresh(refresh_policies::wait_for)
                 .require_alias(true)
                 .wait_for_active_shards("all")
                 .version(7)
                 .version_type(version_types::external_gte)
                 .timeout(std::chrono::seconds{5});

      const auto request = api_request.build();

      EXPECT_EQ(
          request.target(),
          "/books/_doc/42?routing=tenant%2F7&pipeline=normalize&op_type=create"
          "&refresh=wait_for&require_alias=true&wait_for_active_shards=all"
          "&version=7&version_type=external_gte");
      EXPECT_EQ(request.timeout(), std::chrono::seconds{5});
  }

  TEST(IndexDocumentRequest, RejectsEmptyIndex)
  {
      EXPECT_THROW((index_document_request{"", "{}"}), std::invalid_argument);
  }

  TEST(IndexDocumentRequest, RejectsEmptyId)
  {
      EXPECT_THROW((index_document_request{"books", "", "{}"}), std::invalid_argument);
  }

  TEST(IndexDocumentRequest, RejectsEmptyDocument)
  {
      EXPECT_THROW((index_document_request{"books", ""}), std::invalid_argument);
  }

  TEST(IndexDocumentRequest, RejectsNonPositiveTimeout)
  {
      index_document_request request{"books", "{}"};

      EXPECT_THROW(
          request.timeout(std::chrono::milliseconds::zero()),
          std::invalid_argument);
  }
//-------------------------------------------------------------------------//
} // namespace
