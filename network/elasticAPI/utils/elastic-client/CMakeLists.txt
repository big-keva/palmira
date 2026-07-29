set(PROJECT_NAME elastic-client)
project(${PROJECT_NAME})

# Keeps source files.
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads PRIVATE curl)

if (BUILD_TESTS)
    set(TEST_NAME test-elastic-client)

    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    enable_testing()

    # Collecting source test files.
    file(GLOB_RECURSE TEST_SOURCE_FILES src/*.t.cpp)

    add_executable(${TEST_NAME} ${TEST_SOURCE_FILES})
    target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME}
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
        simdjson::simdjson
        z
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
    )

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30)
endif ()
