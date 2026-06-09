# minimal grpc CMake
# protobuf SHOULD come wiht grpc but..
# Loading protobuf first ensures the targets exist.
find_package(protobuf CONFIG QUIET)
find_package(gRPC CONFIG REQUIRED)

# Code-generate C++ from the proto file.
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/imas.proto")

set(PROTO_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTO_GENERATED_DIR}")

set(PROTO_SRCS  "${PROTO_GENERATED_DIR}/imas.pb.cc")
set(PROTO_HDRS  "${PROTO_GENERATED_DIR}/imas.pb.h")
set(GRPC_SRCS   "${PROTO_GENERATED_DIR}/imas.grpc.pb.cc")
set(GRPC_HDRS   "${PROTO_GENERATED_DIR}/imas.grpc.pb.h")

# Locate protoc and grpc_cpp_plugin.
if(TARGET protobuf::protoc)
    set(_protoc     "$<TARGET_FILE:protobuf::protoc>")
    set(_protoc_dep protobuf::protoc)
else()
    find_program(_protoc NAMES protoc REQUIRED)
    set(_protoc_dep "")
endif()

if(TARGET gRPC::grpc_cpp_plugin)
    set(_grpc_plugin     "$<TARGET_FILE:gRPC::grpc_cpp_plugin>")
    set(_grpc_plugin_dep gRPC::grpc_cpp_plugin)
else()
    find_program(_grpc_plugin NAMES grpc_cpp_plugin REQUIRED)
    set(_grpc_plugin_dep "")
endif()

add_custom_command(
    OUTPUT  ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND ${_protoc}
            --proto_path "${CMAKE_CURRENT_SOURCE_DIR}"
            --cpp_out    "${PROTO_GENERATED_DIR}"
            --grpc_out   "${PROTO_GENERATED_DIR}"
            --plugin=protoc-gen-grpc=${_grpc_plugin}
            "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}" ${_protoc_dep} ${_grpc_plugin_dep}
    COMMENT "Generating gRPC C++ sources from imas_backend.proto"
    VERBATIM
)

add_custom_target(imas_grpc_proto_gen
    DEPENDS ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
)

# Attach sources and dependencies to the main `al` library target.
target_sources(al PRIVATE
    grpc_backend.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

add_dependencies(al imas_grpc_proto_gen)

target_include_directories(al PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROTO_GENERATED_DIR}
)

target_compile_definitions(al PRIVATE -DGRPC)
target_link_libraries(al PRIVATE gRPC::grpc++)
