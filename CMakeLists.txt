cmake_minimum_required(VERSION 3.14)
project(T0KVStore)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(PROTO_FILES "${PROTO_DIR}/kvstore.proto")

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
  OUTPUT "${GENERATED_DIR}/kvstore.pb.cc" "${GENERATED_DIR}/kvstore.pb.h"
         "${GENERATED_DIR}/kvstore.grpc.pb.cc" "${GENERATED_DIR}/kvstore.grpc.pb.h"
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
  ARGS --grpc_out=${GENERATED_DIR}
       --cpp_out=${GENERATED_DIR}
       --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`
       -I ${PROTO_DIR}
       ${PROTO_FILES}
  DEPENDS ${PROTO_FILES}
  COMMENT "Running protoc on ${PROTO_FILES}"
)

set(PROTO_SRCS "${GENERATED_DIR}/kvstore.pb.cc" "${GENERATED_DIR}/kvstore.grpc.pb.cc")
set(PROTO_HDRS "${GENERATED_DIR}/kvstore.pb.h" "${GENERATED_DIR}/kvstore.grpc.pb.h")

include_directories(${GENERATED_DIR} ${PROTOBUF_INCLUDE_DIRS} ${GRPC_INCLUDE_DIRS})

include_directories(/usr/include)

link_directories(/usr/lib/aarch64-linux-gnu)

set(CLIENT_SRC ${CMAKE_SOURCE_DIR}/src/739kv.cpp)

add_library(client_lib SHARED ${CLIENT_SRC} ${PROTO_SRCS} ${PROTO_HDRS})
set_target_properties(client_lib PROPERTIES OUTPUT_NAME 739kv)
set_property(TARGET client_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(client_lib gRPC::grpc++ gRPC::grpc ${PROTOBUF_LIBRARIES})

set(TEST_SRC ${CMAKE_SOURCE_DIR}/src/Test.cpp)

add_executable(test ${TEST_SRC})

target_link_libraries(test client_lib gRPC::grpc++ gRPC::grpc ${PROTOBUF_LIBRARIES} leveldb)

set(SERVER_SRC ${CMAKE_SOURCE_DIR}/src/Server.cpp)

add_executable(server ${SERVER_SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(server gRPC::grpc++ gRPC::grpc ${PROTOBUF_LIBRARIES} leveldb)

# Add this section to create the client executable
set(CLIENT_EXEC_SRC ${CMAKE_SOURCE_DIR}/src/MulticlientTest.cpp)

add_executable(client ${CLIENT_EXEC_SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(client client_lib gRPC::grpc++ gRPC::grpc ${PROTOBUF_LIBRARIES})
