#include <chrono>
#include <stdexcept>
#include <type_traits>
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include "elastic/api/client.h"
#include "elastic/api/docapi/delete_document_request.h"
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  using elastic::api::docapi::delete_document_request;
  using elastic::api::docapi::delete_document_response;
  using elastic::api::docapi::refresh_policies;
  using elastic::api::docapi::version_types;

  static_assert(elastic::api::detail::rest_request<delete_document_request>);
  static_assert(std::is_same_v<delete_document_request::response_type, delete_document_response>);
  static_assert(!std::is_copy_constructible_v<delete_document_response>);
  static_assert(std::is_move_constructible_v<delete_document_response>);

  TEST(DeleteDocumentRequest, BuildsDeleteEndpoint)
  {
    const delete_document_request api_request{"books archive", "part/42"};
    const auto request = api_request.build();

    EXPECT_EQ(request.method(), elastic::http::method_types::del);
    EXPECT_EQ(request.target(), "/books%20archive/_doc/part%2F42");
    EXPECT_FALSE(request.has_body());
    EXPECT_TRUE(request.headers().contains("Accept"));
    EXPECT_EQ(request.timeout(), std::chrono::seconds{30});
  }

  TEST(DeleteDocumentRequest, BuildsQueryParameters)
  {
    delete_document_request api_request{"books", "42"};

    api_request.routing("tenant/7")
               .refresh(refresh_policies::wait_for)
               .wait_for_active_shards("all")
               .version(8)
               .version_type(version_types::external)
               .if_sequence_number(11)
               .if_primary_term(3)
               .timeout(std::chrono::seconds{5});

    const auto request = api_request.build();

    EXPECT_EQ(request.target(),
              "/books/_doc/42?routing=tenant%2F7&refresh=wait_for"
              "&wait_for_active_shards=all&version=8&version_type=external"
              "&if_seq_no=11&if_primary_term=3");
    EXPECT_EQ(request.timeout(), std::chrono::seconds{5});
  }

  TEST(DeleteDocumentRequest, RejectsEmptyIndex)
  {
    EXPECT_THROW((delete_document_request{"", "42"}), std::invalid_argument);
  }

  TEST(DeleteDocumentRequest, RejectsEmptyId)
  {
    EXPECT_THROW((delete_document_request{"books", ""}), std::invalid_argument);
  }

  TEST(DeleteDocumentRequest, RejectsEmptyRouting)
  {
    delete_document_request request{"books", "42"};
    EXPECT_THROW(request.routing(""), std::invalid_argument);
  }

  TEST(DeleteDocumentRequest, RejectsNonPositiveTimeout)
  {
    delete_document_request request{"books", "42"};

    EXPECT_THROW(request.timeout(std::chrono::milliseconds::zero()), std::invalid_argument);
  }
//-------------------------------------------------------------------------//
} // namespace
