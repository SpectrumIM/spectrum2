cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)
FILE(GLOB_RECURSE SWIFTEN_SRC ../include/Swiften/*.cpp)
FILE(GLOB HEADERS ../include/transport/*.h)

if (CPPUNIT_FOUND)
	FILE(GLOB SRC_TEST tests/*.cpp)

	ADD_EXECUTABLE(libtransport_test ${SRC_TEST})

	target_link_libraries(libtransport_test transport ${CPPUNIT_LIBRARIES} ${Boost_LIBRARIES})
endif()

include_directories(${POPT_INCLUDE_DIR})

# SOURCE_GROUP(headers FILES ${HEADERS})



if (PROTOBUF_FOUND)
    ADD_LIBRARY(transport SHARED ${HEADERS} ${SRC} ${SWIFTEN_SRC} ${CMAKE_CURRENT_BINARY_DIR}/../include/transport/protocol.pb.cc)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)
    ADD_DEPENDENCIES(transport pb) 
else()
    ADD_LIBRARY(transport SHARED ${HEADERS} ${SRC} ${SWIFTEN_SRC})
endif()

ADD_DEFINITIONS(-fPIC)

TARGET_LINK_LIBRARIES(transport ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${MYSQL_LIBRARIES} ${SWIFTEN_LIBRARY} ${PROTOBUF_LIBRARIES} ${LOG4CXX_LIBRARIES} ${POPT_LIBRARY})

SET_TARGET_PROPERTIES(transport PROPERTIES
      VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

INSTALL(TARGETS transport LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)

#CONFIGURE_FILE(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
#INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)
