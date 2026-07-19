set(PROJECT_NAME elasticAPI)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ELASTIC_ENABLE_SSL "Enable SSL support through uWebSockets" OFF)
option(BUILD_USOCKETS_ENABLED "Build usockets library" ON)
option(BUILD_TESTS "Building tests" ON)

# Checking dependencies on available.
find_package(Threads REQUIRED)
find_package(simdjson REQUIRED)

if (ELASTIC_ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
endif (ELASTIC_ENABLE_SSL)


add_compile_options(-DELASTIC_API_ENABLED)

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
    target_include_directories(${USOCKETS_NAME}
        PRIVATE ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/src
        ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src
    )

    if (ELASTIC_SSL_ENABLED)
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
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/src ${CMAKE_SOURCE_DIR}/contrib/uWebSockets/uSockets/src)

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

if (ELASTIC_ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ELASTIC_ENABLE_SSL=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC ELASTIC_ENABLE_SSL=0)
endif ()

if (BUILD_TESTS)
    set(TEST_NAME test-elastic-api)

    find_package(GTest REQUIRED)
    find_package(Threads REQUIRED)

    enable_testing()

    # Collecting source test files.
    file(GLOB_RECURSE TEST_SOURCE_FILES src/*.t.cpp)

    add_executable(${TEST_NAME} ${TEST_SOURCE_FILES})
    target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME}
        tripoli
        structo
        mtc
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
        simdjson::simdjson
        z
    )
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30)
endif ()
