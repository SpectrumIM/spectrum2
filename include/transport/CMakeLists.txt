if(PROTOBUF_FOUND)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/protocol.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/protocol.pb.h
		COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out  ${CMAKE_CURRENT_SOURCE_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/protocol.proto
		COMMENT "Running C++ protocol buffer compiler on protocol.proto"
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol.proto
	)
	add_custom_target(pb DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol.pb.cc)
endif()

file(GLOB HEADERS *.h protocol.h)

install(FILES ${HEADERS} DESTINATION include/transport COMPONENT headers)
