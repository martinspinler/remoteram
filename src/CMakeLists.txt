# SPDX-License-Identifier: GPL-2.0
#
# CMake build file for NFB debug tools
#
# Copyright (C) 2022 CESNET
# Author(s):
#   Martin Spinler <spinler@cesnet.cz>
#

cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)
project(remoteram-grpc C CXX)
include(GNUInstallDirs)

include(CPack)

find_package(Protobuf CONFIG)
#find_package(gRPC CONFIG)
set(PROTO_FILES protobuf/remoteram.proto)

add_library(remoteram SHARED remoteram_server.cc)

if (EXISTS ${Protobuf_DIR})
	find_package(gRPC CONFIG)

	include_directories(${Protobuf_INCLUDE_DIRS})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	target_sources(remoteram PRIVATE ${PROTO_FILES})

	get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
	protobuf_generate(TARGET remoteram LANGUAGE cpp)
	protobuf_generate(TARGET remoteram LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

	set(gRPC_LIB_PREFIX "gRPC::")
else()
	set(protobuf_MODULE_COMPATIBLE TRUE)

	find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
	find_program(_PROTOBUF_PROTOC protoc)

	get_filename_component(remoteram_proto ${PROTO_FILES} ABSOLUTE)
	get_filename_component(remoteram_proto_path "${remoteram_proto}" PATH)

	set(remoteram_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/remoteram.pb.cc")
	set(remoteram_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/remoteram.pb.h")
	set(remoteram_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/remoteram.grpc.pb.cc")
	set(remoteram_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/remoteram.grpc.pb.h")

	add_custom_command(
		  OUTPUT "${remoteram_proto_srcs}" "${remoteram_proto_hdrs}" "${remoteram_grpc_srcs}" "${remoteram_grpc_hdrs}"
		  COMMAND ${_PROTOBUF_PROTOC}
		  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
			--cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
			-I "${remoteram_proto_path}"
			--plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
			"${remoteram_proto}"
		  DEPENDS "${remoteram_proto}")

	include_directories("${CMAKE_CURRENT_BINARY_DIR}")

	set(PROTO_OUT_FILES ${remoteram_grpc_srcs} ${remoteram_grpc_hdrs} ${remoteram_proto_srcs} ${remoteram_proto_hdrs})
	set(gRPC_LIB_PREFIX "")
	set(Protobuf_LIBRARIES "protobuf")
endif()

target_sources(remoteram PRIVATE ${PROTO_OUT_FILES})

target_link_libraries(remoteram ${Protobuf_LIBRARIES} ${gRPC_LIB_PREFIX}grpc++)

set_target_properties(remoteram PROPERTIES
	OUTPUT_NAME remoteram
	VERSION 1.0.0
	SOVERSION 1
	PUBLIC_HEADER "remoteram.h"
)

install(TARGETS remoteram
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER
	# FIXME
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/remoteram/
)

install(DIRECTORY protobuf
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/remoteram/
)
