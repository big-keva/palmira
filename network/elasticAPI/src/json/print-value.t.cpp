#include "print-value.h"
//-------------------------------------------------------------------------//
#include <gtest/gtest.h>
//-------------------------------------------------------------------------//
namespace mtc::json
{
//-------------------------------------------------------------------------//
  template<typename output_t, typename value_t>
  auto Print(output_t *o, value_t val) -> output_t *
  {
    return o;
  }
}
//-------------------------------------------------------------------------//
namespace
{
//-------------------------------------------------------------------------//
  TEST(TestPrintValue, printValueObject)
  {
    const auto zjson = mtc::zmap{{"_source", mtc::array_zval{
      mtc::zmap{{"title", "Первая запись"}},
      mtc::zmap{{"author", "Иван Иванович"}},
      mtc::zmap{{"content", "Это моя первая запись в блоге."}}
    }}};

/*
    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::dump(&buffer, zjson.get_array_zval("_source", {})));
    EXPECT_EQ(buffer.str(), R"("title":"Первая запись","author":"Иван Иванович","content":"Это моя первая запись в блоге.")");
*/
  }

  TEST(TestPrintValue, printValueArrayInt)
  {
    const auto zjson = mtc::zmap{{"array", mtc::array_zval{
      1, 2, 3, 4, 5, 6, 7, 8, 9
    }}};

/*
    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::dump(&buffer, zjson.get_array_zval("array", {})));
    EXPECT_EQ(buffer.str(), R"(1,2,3,4,5,6,7,8,9)");
*/
  }

  TEST(TestPrintValue, printValueArrayString)
  {
    const auto zjson = mtc::zmap{{"array", mtc::array_zval{
      "1", "2", "3", "4", "5", "6", "7", "8", "9"
    }}};

/*
    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::dump(&buffer, zjson.get_array_zval("array", {})));
    EXPECT_EQ(buffer.str(), R"("1","2","3","4","5","6","7","8","9")");
 */
  }

  TEST(TestPrintValue, printValueComplex)
  {
    const auto zjson = mtc::zmap{
      {"array", mtc::array_zval{"1", "2", "3", "4", "5", "6", "7", "8", "9"}},
    {"first_name", "test_first_name"},
      {"last_name", "test_last_name"}
    };

/*
    std::ostringstream buffer;
    ASSERT_NO_THROW(elastic::json::dump(&buffer, &zjson));
    EXPECT_EQ(buffer.str(), R"("array":["1","2","3","4","5","6","7","8","9"],"first_name":"test_first_name","last_name":"test_last_name")");
*/
  }
//-------------------------------------------------------------------------//
} // namespace
