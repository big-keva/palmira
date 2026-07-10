set(PROJECT_NAME test-docapi-json)
project(${PROJECT_NAME})

find_package(GTest REQUIRED)

# Collecting source test files.
file(GLOB_RECURSE TEST_JSON_SOURCE *.t.cpp)

add_executable(${PROJECT_NAME} ${TEST_JSON_SOURCE})
target_link_libraries(${PROJECT_NAME} PRIVATE
    docapi-http
    GTest::gtest
    GTest::gtest_main
    simdjson::simdjson
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
