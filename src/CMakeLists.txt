cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)
FILE(GLOB_RECURSE SWIFTEN_SRC ../include/Swiften/*.cpp)

# Build without openssl on msvc
if (CMAKE_COMPILER_IS_GNUCXX)
	string(REGEX REPLACE "[^;]+;?/Schannel/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}") 
else()
	string(REGEX REPLACE "[^;]+;?/OpenSSL/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}") 
endif()

FILE(GLOB HEADERS ../include/transport/*.h)

if (CPPUNIT_FOUND)
	FILE(GLOB SRC_TEST tests/*.cpp)

	ADD_EXECUTABLE(libtransport_test ${SRC_TEST})
	set_target_properties(libtransport_test PROPERTIES COMPILE_DEFINITIONS LIBTRANSPORT_TEST=1)

	target_link_libraries(libtransport_test transport ${CPPUNIT_LIBRARY} ${Boost_LIBRARIES})
endif()

if (NOT WIN32)
include_directories(${POPT_INCLUDE_DIR})
endif()

# SOURCE_GROUP(headers FILES ${HEADERS})


if (PROTOBUF_FOUND)
	if (NOT WIN32)
		ADD_LIBRARY(transport SHARED ${HEADERS} ${SRC} ${SWIFTEN_SRC})
	else()
		ADD_LIBRARY(transport STATIC ${HEADERS} ${SRC} ${SWIFTEN_SRC})
	endif()
#	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)
	ADD_DEPENDENCIES(transport pb) 
else(PROTOBUF_FOUND)
	ADD_LIBRARY(transport SHARED ${HEADERS} ${SRC} ${SWIFTEN_SRC})
endif(PROTOBUF_FOUND)

# if (CMAKE_COMPILER_IS_GNUCXX)
	if (NOT WIN32)
		ADD_DEFINITIONS(-fPIC)
	endif()
# endif()

if (WIN32)
	include_directories("${CMAKE_SOURCE_DIR}/msvc-deps/sqlite3")
	TARGET_LINK_LIBRARIES(transport transport-plugin sqlite3 ${PQXX_LIBRARY} ${PQ_LIBRARY} ${MYSQL_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${PROTOBUF_LIBRARY})
else()
	TARGET_LINK_LIBRARIES(transport transport-plugin ${PQXX_LIBRARY} ${PQ_LIBRARY} ${SQLITE3_LIBRARIES} ${MYSQL_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${POPT_LIBRARY} ${PROTOBUF_LIBRARY})
endif()

SET_TARGET_PROPERTIES(transport PROPERTIES
      VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

INSTALL(TARGETS transport LIBRARY DESTINATION ${LIB_INSTALL_DIR} ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries)

#CONFIGURE_FILE(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
#INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)
