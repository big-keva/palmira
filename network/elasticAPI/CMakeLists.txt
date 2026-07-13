set(PROJECT_NAME elasticAPI)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ELASTIC_ENABLE_SSL "Enable SSL support through uWebSockets" ON)
option(BUILD_TESTS "Building unit-tests" ON)

set(UWEBSOCKETS_DIR ../../src/network/elastic/thirdparty/uWebSockets)
set(USOCKETS_DIR ${UWEBSOCKETS_DIR}/uSockets)

# Checking dependencies on available.
find_package(Threads REQUIRED)
find_package(simdjson REQUIRED)

if (ELASTIC_ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
endif (ELASTIC_ENABLE_SSL)

# Keeps source files.
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
# Removing unit-tests files from target module.
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*\\.t\\.cpp$")

# Building a module.
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/src
    ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC Threads::Threads
           simdjson::simdjson
    PRIVATE tripoli
            structo
            DeliriX
            ${MoonyCode_LIB}
            mtc
            tinyxml2
            minizip
            z
            pthread
            dl
            usockets
            z
)

if (ELASTIC_ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ELASTIC_ENABLE_SSL=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC ELASTIC_ENABLE_SSL=0)
endif ()

if (BUILD_TESTS)
    enable_testing()

    find_package(GTest REQUIRED)

    # Collecting source test files.
    file(GLOB_RECURSE TEST_SOURCE_FILES src/*.t.cpp)

    add_executable(test-elastic ${TEST_SOURCE_FILES})
    target_link_libraries(test-elastic PRIVATE
        ${PROJECT_NAME}
        GTest::gtest
        GTest::gtest_main
        simdjson::simdjson
    )

    add_test(NAME test-elastic COMMAND test-elastic)
endif ()
