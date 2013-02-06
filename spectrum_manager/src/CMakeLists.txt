cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.c)

ADD_EXECUTABLE(spectrum2_manager ${SRC} ../../src/config.cpp ../../src/util.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.pb.cc)

ADD_DEPENDENCIES(spectrum2_manager pb)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)

target_link_libraries(spectrum2_manager ${SWIFTEN_LIBRARY} ${PROTOBUF_LIBRARIES})

if(APPLE)
target_link_libraries(spectrum2_manager ${APPLE_FRAMEWORKS})
endif()
INSTALL(TARGETS spectrum2_manager RUNTIME DESTINATION bin)

INSTALL(FILES
	spectrum_manager.cfg
	DESTINATION /etc/spectrum2
	)
