#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
#include "elastic/api/client.h"
#include "elastic/api/docapi/get_document_request.h"
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  using elastic::api::docapi::get_document_request;
  using elastic::api::docapi::get_document_response;
  using elastic::api::docapi::version_type;
//-------------------------------------------------------------------------//
  static_assert(elastic::api::detail::rest_request<get_document_request>);
  static_assert(std::is_same_v<get_document_request::response_type, get_document_response>);
  static_assert(!std::is_copy_constructible_v<get_document_response>);
  static_assert(std::is_move_constructible_v<get_document_response>);
//-------------------------------------------------------------------------//
  TEST(GetDocumentRequest, BuildsMinimalRequest)
  {
      const get_document_request api_request{"books", "42"};
      const auto request = api_request.build();

      EXPECT_EQ(request.method(), elastic::http::method_types::get);
      EXPECT_EQ(request.target(), "/books/_doc/42");
      EXPECT_EQ(request.timeout(), std::chrono::seconds{30});
      EXPECT_TRUE(request.headers().contains("Accept"));
  }

  TEST(GetDocumentRequest, EncodesPathSegments)
  {
      const get_document_request api_request{"books archive", "part/42"};
      const auto request = api_request.build();

      EXPECT_EQ(request.target(), "/books%20archive/_doc/part%2F42");
  }

  TEST(GetDocumentRequest, BuildsQueryParameters)
  {
      get_document_request api_request{"books", "42"};

      api_request.routing("tenant/7")
                 .preference("_local")
                 .realtime(false)
                 .refresh(true)
                 .source(true)
                 .source_includes({"title", "author.name"})
                 .source_excludes({"private"})
                 .stored_fields({"title", "year"})
                 .version(7)
                 .version_type(version_type::external_gte)
                 .timeout(std::chrono::seconds{5});

      const auto request = api_request.build();

      EXPECT_EQ(
          request.target(),
          "/books/_doc/42?routing=tenant%2F7&preference=_local&realtime=false"
          "&refresh=true&_source=true&_source_includes=title%2Cauthor.name"
          "&_source_excludes=private&stored_fields=title%2Cyear&version=7"
          "&version_type=external_gte");
      EXPECT_EQ(request.timeout(), std::chrono::seconds{5});
  }

  TEST(GetDocumentRequest, RejectsEmptyIndex)
  {
      EXPECT_THROW((get_document_request{"", "42"}), std::invalid_argument);
  }

  TEST(GetDocumentRequest, RejectsEmptyId)
  {
      EXPECT_THROW((get_document_request{"books", ""}), std::invalid_argument);
  }

  TEST(GetDocumentRequest, RejectsNonPositiveTimeout)
  {
      get_document_request request{"books", "42"};

      EXPECT_THROW(request.timeout(std::chrono::milliseconds::zero()), std::invalid_argument);
  }
//-------------------------------------------------------------------------//
} // namespace
