cmake_minimum_required(VERSION 2.6)

if (PROTOBUF_FOUND)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/protocol_pb2.py
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --python_out  ${CMAKE_CURRENT_SOURCE_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/ ${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.proto
        COMMENT "Running Python protocol buffer compiler on protocol.proto"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.proto
    )
    ADD_CUSTOM_TARGET(pb-python ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol_pb2.py)
endif()




