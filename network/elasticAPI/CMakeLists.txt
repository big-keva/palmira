set(PROJECT_NAME elasticAPI)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ELASTIC_SSL_ENABLED "Enable SSL support through uWebSockets" ON)
option(BUILD_USOCKETS_ENABLED "Build usockets library" ON)
option(BUILD_TESTS "Building tests" ON)

# Checking dependencies on available.
find_package(Threads REQUIRED)
find_package(simdjson REQUIRED)

if (ELASTIC_ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
endif (ELASTIC_ENABLE_SSL)

add_compile_options(-DELASTIC_API_ENABLED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-D__DEBUG__)
endif ()

if (BUILD_USOCKETS_ENABLED)
    set(USOCKETS_NAME usockets)
    set(USOCKETS_SOURCES
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/bsd.c
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/context.c
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/loop.c
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/socket.c
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/eventing/epoll_kqueue.c
    )

    if (ELASTIC_SSL_ENABLED)
        list(APPEND USOCKETS_SOURCES
            ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/crypto/openssl.c
            ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src/crypto/sni_tree.cpp
        )
    endif (ELASTIC_SSL_ENABLED)

    add_library(${USOCKETS_NAME} STATIC ${USOCKETS_SOURCES})
    set_target_properties(${USOCKETS_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(${USOCKETS_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/src
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src
    )

    if (ELASTIC_SSL_ENABLED)
        find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

        target_compile_definitions(${USOCKETS_NAME} PUBLIC LIBUS_USE_OPENSSL)
        target_link_libraries(${USOCKETS_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    else ()
        target_compile_definitions(${USOCKETS_NAME} PUBLIC LIBUS_NO_SSL)
    endif (ELASTIC_SSL_ENABLED)
endif (BUILD_USOCKETS_ENABLED)

# Keeps source files.
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
# Removing unit-tests files from target module.
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*\\.t\\.cpp$")

# Building a module.
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/src
    ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads simdjson::simdjson PRIVATE
    tripoli
    structo
    DeliriX
    ${MoonyCode_LIB}
    mtc
    tinyxml2
    minizip
    Threads::Threads
    dl
    usockets
    z
)

if (BUILD_TESTS)
    set(TEST_PROJECT_NAME test-elastic-api)

    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    enable_testing()

    #
    add_subdirectory(utils)

    # Collecting source test files.
    file(GLOB_RECURSE TEST_SOURCE_FILES src/*.t.cpp)

    add_executable(${TEST_PROJECT_NAME} ${TEST_SOURCE_FILES})
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE ${PROJECT_NAME}
        elastic-client
        tripoli
        structo
        mtc
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
        simdjson::simdjson
        z
    )

    if (ELASTIC_SSL_ENABLED)
        target_link_libraries(${TEST_PROJECT_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    endif (ELASTIC_SSL_ENABLED)

    target_include_directories(${TEST_PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/utils/elastic-client/include
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/src
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src
    )

    add_test(NAME ${TEST_PROJECT_NAME} COMMAND ${TEST_PROJECT_NAME})
    set_tests_properties(${TEST_PROJECT_NAME} PROPERTIES TIMEOUT 30)
endif ()
