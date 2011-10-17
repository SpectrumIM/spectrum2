if (PROTOBUF_FOUND)
	add_custom_target(pb
		${PROTOBUF_PROTOC_EXECUTABLE}
		--cpp_out  ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/protocol.proto
		COMMENT "Running C++ protocol buffer compiler on protocol.proto"
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/protocol.proto
		VERBATIM )
endif()

FILE(GLOB HEADERS *.h protocol.h)

INSTALL(FILES ${HEADERS} DESTINATION include/transport COMPONENT headers)