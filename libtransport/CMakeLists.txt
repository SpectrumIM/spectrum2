file(GLOB SRC *.cpp *.h)
file(GLOB_RECURSE SWIFTEN_SRC ../include/Swiften/*.cpp)

# Build without openssl on msvc
if(NOT MSVC)
	if(APPLE)
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

file(GLOB HEADERS ../include/transport/*.h)
include_directories(../spectrum/src/frontends/xmpp/)

if(NOT WIN32)
	include_directories(${POPT_INCLUDE_DIR})
endif()

#source_group(headers FILES ${HEADERS})

if(PROTOBUF_FOUND)
	if(NOT WIN32)
		add_library(transport SHARED ${HEADERS} ${SRC} ${SWIFTEN_SRC})
	else()
		add_library(transport STATIC ${HEADERS} ${SRC} ${SWIFTEN_SRC})
	endif()
#	set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)
	add_dependencies(transport pb) 
else(PROTOBUF_FOUND)
	add_library(transport SHARED ${HEADERS} ${SRC} ${SWIFTEN_SRC})
endif(PROTOBUF_FOUND)

# if(CMAKE_COMPILER_IS_GNUCXX)
	if(NOT WIN32)
		add_definitions(-fPIC)
	endif()
# endif()

find_package(CURL)

target_link_libraries(transport transport-plugin ${CURL_LIBRARIES} ${SQLITE3_LIBRARIES} ${MYSQL_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${POPT_LIBRARY} ${PROTOBUF_LIBRARY} JsonCpp::JsonCpp)

if(WIN32)
	target_link_libraries(transport psapi.lib bcrypt.lib)
endif()

if(PQXX_FOUND)
	target_link_libraries(transport	${PQXX_LIBRARIES} ${PQ_LIBRARY})
endif()

set_target_properties(transport PROPERTIES
	VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)
if(APPLE)
	target_link_libraries(transport ${APPLE_FRAMEWORKS})
endif()

install(TARGETS transport LIBRARY DESTINATION ${LIB_INSTALL_DIR} ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries)

#configure_file(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
#install(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)
