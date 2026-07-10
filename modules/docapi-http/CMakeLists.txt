set(PROJECT_NAME docapi-http)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DOCAPI_BUILD_SHARED "Build module as shared library" ON)
option(DOCAPI_ENABLE_SSL "Enable SSL support through uWebSockets" ON)
option(BUILD_TESTS "Building unit-tests" ON)

set(BUILD_MODULE_TYPE SHARED)
set(UWEBSOCKETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/uWebSockets)
set(USOCKETS_DIR ${UWEBSOCKETS_DIR}/uSockets)

# Checking dependencies on available.
find_package(Threads REQUIRED)
find_package(simdjson REQUIRED)

if (DOCAPI_BUILD_SHARED)
    set(BUILD_MODULE_TYPE SHARED)
else ()
    set(BUILD_MODULE_TYPE STATIC)
endif (DOCAPI_BUILD_SHARED)

if (DOCAPI_ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
endif (DOCAPI_ENABLE_SSL)

# Building third party modules.
add_subdirectory(thirdparty)

# Keeps source files.
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
# Removing unit-tests files from target module.
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*\\.t\\.cpp$")

# Building a module.
add_library(${PROJECT_NAME} ${BUILD_MODULE_TYPE} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${UWEBSOCKETS_DIR}/src
        ${USOCKETS_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/include
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Threads::Threads
        simdjson::simdjson

    PRIVATE
        tripoli
        structo
        DeliriX
        ${MoonyCode_LIB}
        modules-common
        mtc
        tinyxml2
        minizip
        z
        pthread
        dl
        usockets
        z
)

if (DOCAPI_ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCAPI_ENABLE_SSL=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCAPI_ENABLE_SSL=0)
endif ()

if (BUILD_TESTS)
    enable_testing()

    add_subdirectory(src/json)
endif ()