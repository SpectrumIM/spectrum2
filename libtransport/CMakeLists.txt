cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)
FILE(GLOB_RECURSE SWIFTEN_SRC ../include/Swiften/*.cpp)

# Build without openssl on msvc
if (NOT MSVC)
if (APPLE)
	string(REGEX REPLACE "[^;]+;?/Schannel/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
	string(REGEX REPLACE "[^;]+;?/OpenSSL/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
else()
	string(REGEX REPLACE "[^;]+;?/Schannel/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
	string(REGEX REPLACE "[^;]+;?/SecureTransport/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}") 
endif()
else()
	string(REGEX REPLACE "[^;]+;?/OpenSSL/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}") 
	string(REGEX REPLACE "[^;]+;?/SecureTransport/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
endif()
FILE(GLOB HEADERS ../include/transport/*.h)
include_directories(../spectrum/src/frontends/xmpp/)

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

find_package(CURL)

if (WIN32)
	include_directories("${CMAKE_SOURCE_DIR}/msvc-deps/sqlite3")
	TARGET_LINK_LIBRARIES(transport transport-plugin sqlite3 ${PQXX_LIBRARY} ${CURL_LIBRARIES} ${PQ_LIBRARY} ${MYSQL_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${PROTOBUF_LIBRARY} psapi.lib)
else()
	TARGET_LINK_LIBRARIES(transport transport-plugin ${PQXX_LIBRARY} ${CURL_LIBRARIES} ${PQ_LIBRARY} ${SQLITE3_LIBRARIES} ${MYSQL_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${POPT_LIBRARY} ${PROTOBUF_LIBRARY})
endif()

SET_TARGET_PROPERTIES(transport PROPERTIES
      VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)
if (APPLE)
	TARGET_LINK_LIBRARIES(transport ${APPLE_FRAMEWORKS})
endif()

INSTALL(TARGETS transport LIBRARY DESTINATION ${LIB_INSTALL_DIR} ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries)

#CONFIGURE_FILE(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
#INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)
