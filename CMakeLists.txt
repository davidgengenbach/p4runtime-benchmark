cmake_minimum_required(VERSION 3.17)
project(p4runtime_benchmark)

set(CMAKE_CXX_STANDARD 14)


find_package(PkgConfig REQUIRED)

list(
        APPEND
        LIBRARIES
        protobuf_generated_p4
)

foreach (
        LIB
        grpc++
        protobuf
        grpc
)
    pkg_check_modules(${LIB} REQUIRED IMPORTED_TARGET ${LIB})
    list(APPEND LIBRARIES PkgConfig::${LIB})
endforeach ()


set(PROTO_COMPILED_DIR ${PROJECT_SOURCE_DIR}/third_party/proto_compiled)
include_directories(${PROTO_COMPILED_DIR})
add_library(
        protobuf_generated_p4
        STATIC
        ${PROTO_COMPILED_DIR}/p4/v1/p4runtime.grpc.pb.cc
        ${PROTO_COMPILED_DIR}/p4/v1/p4runtime.pb.cc
        ${PROTO_COMPILED_DIR}/p4/v1/p4data.pb.cc
        ${PROTO_COMPILED_DIR}/p4/v1/p4data.grpc.pb.cc
        ${PROTO_COMPILED_DIR}/google/rpc/status.grpc.pb.cc
        ${PROTO_COMPILED_DIR}/google/rpc/status.pb.cc
        ${PROTO_COMPILED_DIR}/p4/config/v1/p4info.grpc.pb.cc
        ${PROTO_COMPILED_DIR}/p4/config/v1/p4info.pb.cc
        ${PROTO_COMPILED_DIR}/p4/config/v1/p4types.grpc.pb.cc
        ${PROTO_COMPILED_DIR}/p4/config/v1/p4types.pb.cc
)

add_executable(
        p4runtime_benchmark
        main.cpp
)

target_link_libraries(
        p4runtime_benchmark
        ${LIBRARIES}
)