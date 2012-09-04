if (PROTOBUF_FOUND)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/protocol.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/protocol.pb.h
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out  ${CMAKE_CURRENT_SOURCE_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/protocol.proto
        COMMENT "Running C++ protocol buffer compiler on protocol.proto"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol.proto
    )
    ADD_CUSTOM_TARGET(pb DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol.pb.cc)
endif()

FILE(GLOB HEADERS *.h protocol.h)

INSTALL(FILES ${HEADERS} DESTINATION include/transport COMPONENT headers)