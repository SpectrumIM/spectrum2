FILE(GLOB SRC *.cpp)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pbnetwork_pb2.py"
	COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
	ARGS --python_out  ${CMAKE_CURRENT_BINARY_DIR} pbnetwork.proto
	COMMENT "Running Py protocol buffer compiler on pbnetwork.proto"
	VERBATIM )

ADD_EXECUTABLE(external_network_plugin ${SRC} ${CMAKE_CURRENT_BINARY_DIR}/pbnetwork_pb2.py)

TARGET_LINK_LIBRARIES(external_network_plugin transport ${SWIFTEN_LIBRARIES})

