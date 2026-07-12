set(PROJECT_NAME networkapi)
project(${PROJECT_NAME})

add_library(${PROJECT_NAME} INTERFACE
    elastic-server.hpp
    grpc-client.hpp
    grpc-server.hpp
    http-client.hpp
    http-server.hpp
)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
