#include "serializer.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  TEST(TestSerializer, serialize)
  {
    const auto zjson = mtc::zmap{{"_source", mtc::array_zval{
      mtc::zmap{{"title", "Первая запись"}},
      mtc::zmap{{"author", "Иван Иванович"}},
      mtc::zmap{{"content", "Это моя первая запись в блоге."}}
    }}};

    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::serialize(buffer, zjson.get_array_zval("_source", {})));
    EXPECT_EQ(buffer.str(), R"("title":"Первая запись","author":"Иван Иванович","content":"Это моя первая запись в блоге.")");
  }
//-------------------------------------------------------------------------//
} // namespace
