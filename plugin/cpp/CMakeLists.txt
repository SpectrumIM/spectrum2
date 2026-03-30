file(GLOB SRC *.cpp *.h)
file(GLOB HEADERS ../include/transport/*.h)

set(EXTRA_SOURCES ${EXTRA_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.pb.cc)

if(NOT WIN32)
	add_library(transport-plugin SHARED ${HEADERS} ${SRC} ${PROTOBUF_SRC} ${PROTOBUF_HDRS} ${EXTRA_SOURCES})
else()
	add_library(transport-plugin STATIC ${HEADERS} ${SRC} ${EXTRA_SOURCES} )
endif()

add_dependencies(transport-plugin pb)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)
target_compile_features(transport-plugin PUBLIC cxx_std_11)

if(CMAKE_COMPILER_IS_GNUCXX)
	if(NOT WIN32)
		add_definitions(-fPIC)
	endif()
endif()

if(NOT WIN32)
	target_link_libraries(transport-plugin ${PROTOBUF_LIBRARY} ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES})
else()
	target_link_libraries(transport-plugin ${PROTOBUF_LIBRARY} ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES} ws2_32.lib)
endif() 

set_target_properties(transport-plugin PROPERTIES
	VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

install(TARGETS transport-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)

#configure_file(transport.pc.in "${CMAKE_CURRENT_SOURCE_DIR}/transport.pc")
#install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/transport.pc" DESTINATION lib/pkgconfig)
