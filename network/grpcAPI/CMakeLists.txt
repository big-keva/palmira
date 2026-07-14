project(restAPI)

include_directories(contrib)

set(TARGET
  palmira-gRPC)

include(src/cmake/grpc.cmake)

if(MSVC)
  set(EXPORT_TABLE ${NETWORK_SOURCE_DIR}/network.msvc.def)
else()
  set(SYMBOLS_FILE ${NETWORK_SOURCE_DIR}/network.unix.def)

  add_compile_options(-W -Wall -Wextra)
  add_compile_options(-fPIC)

  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0)
  else()
    add_compile_options(-fstack-protector-all -g)
  endif()

  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--dynamic-list=${SYMBOLS_FILE} -static-libgcc -static-libstdc++" )
endif()

set(PROTO ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/grpcttp.proto)

set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.pb.h")
set(PROTO_GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.grpc.pb.cc")
set(PROTO_GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.grpc.pb.h")

set(PROTOBUF_PROTOC protoc)

set(GRPC_SRC
  src/messages.cpp
  src/server.cpp
  src/client.cpp)

add_custom_command(
  OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${PROTO_GRPC_SRC}" "${PROTO_GRPC_HDR}"
  COMMAND ${PROTOBUF_PROTOC}
  ARGS
    --grpc_out   "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out    "${CMAKE_CURRENT_BINARY_DIR}"
    --proto_path "${CMAKE_CURRENT_SOURCE_DIR}/src/proto"
    --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
    "${PROTO}"
  DEPENDS "${PROTO}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(${TARGET} SHARED
  ${GRPC_SRC}
  ${PROTO_SRC}
  ${PROTO_HDR}
  ${PROTO_GRPC_SRC}
  ${PROTO_GRPC_HDR})

target_link_libraries(${TARGET}
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  structo
  DeliriX
  moonycode
  mtc
  z)
