#include "serializer.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  TEST(TestSerializer, serializeObject)
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

  TEST(TestSerializer, serializeArrayInt)
  {
    const auto zjson = mtc::zmap{{"array", mtc::array_zval{
      1, 2, 3, 4, 5, 6, 7, 8, 9
    }}};

    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::serialize(buffer, zjson.get_array_zval("array", {})));
    EXPECT_EQ(buffer.str(), R"(1,2,3,4,5,6,7,8,9)");
  }

  TEST(TestSerializer, serializeArrayString)
  {
    const auto zjson = mtc::zmap{{"array", mtc::array_zval{
      "1", "2", "3", "4", "5", "6", "7", "8", "9"
    }}};

    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::serialize(buffer, zjson.get_array_zval("array", {})));
    EXPECT_EQ(buffer.str(), R"("1","2","3","4","5","6","7","8","9")");
  }
//-------------------------------------------------------------------------//
} // namespace
