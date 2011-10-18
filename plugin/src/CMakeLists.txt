cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)
FILE(GLOB HEADERS ../include/transport/*.h)

ADD_LIBRARY(transport-plugin SHARED ${HEADERS} ${SRC} ${PROTOBUF_SRC} ${PROTOBUF_HDRS} ../../src/memoryusage.cpp ../../include/transport/protocol.pb.cc)
ADD_DEPENDENCIES(transport-plugin pb)
ADD_DEFINITIONS(-fPIC)

TARGET_LINK_LIBRARIES(transport-plugin ${PROTOBUF_LIBRARIES} ${LOG4CXX_LIBRARIES})

SET_TARGET_PROPERTIES(transport-plugin PROPERTIES
      VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

INSTALL(TARGETS transport-plugin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)

#CONFIGURE_FILE(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
#INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)
