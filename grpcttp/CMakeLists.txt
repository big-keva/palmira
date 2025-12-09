cmake_minimum_required(VERSION 3.16)

project(grpcttp C CXX)

include(cmake/grpc.cmake)

set(PROTO ${PROJECT_SOURCE_DIR}/proto/grpcttp.proto)

set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.pb.h")
set(PROTO_GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.grpc.pb.cc")
set(PROTO_GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/grpcttp.grpc.pb.h")

set(SRC src/messages.cpp)

set(PROTOBUF_PROTOC protoc)

message(WARNING "${PROJECT_SOURCE_DIR}")

add_custom_command(
      OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${PROTO_GRPC_SRC}" "${PROTO_GRPC_HDR}"
      COMMAND ${PROTOBUF_PROTOC}
      ARGS
        --grpc_out   "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out    "${CMAKE_CURRENT_BINARY_DIR}"
        --proto_path "${PROJECT_SOURCE_DIR}/proto"
        --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
        "${PROTO}"
      DEPENDS "${PROTO}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(grpcttp
	${SRC}
	${PROTO_SRC}
	${PROTO_HDR}
	${PROTO_GRPC_SRC}
	${PROTO_GRPC_HDR})

target_link_libraries(grpcttp
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

#target_link_libraries(grpcttp-server
#  omicron
#  hw_grpc_proto
#  absl::check
#  absl::flags
#  absl::flags_parse
#  absl::log
#  absl::log_initialize
#  ${_REFLECTION}
#  ${_GRPC_GRPCPP}
#  ${_PROTOBUF_LIBPROTOBUF})
#
#target_link_libraries(grpcttp-client
#  omicron
#  moonycode
#  mtc
#  hw_grpc_proto
#  absl::check
#  absl::flags
#  absl::flags_parse
#  absl::log
#  absl::log_initialize
#  ${_REFLECTION}
#  ${_GRPC_GRPCPP}
#  ${_PROTOBUF_LIBPROTOBUF})
