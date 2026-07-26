file(GLOB SRC *.cpp *.h)
file(GLOB HEADERS ../include/transport/*.h)


if(NOT WIN32)
	add_library(transport-plugin SHARED ${HEADERS} ${SRC})
else()
	add_library(transport-plugin STATIC ${HEADERS} ${SRC} )
endif()

add_dependencies(transport-plugin pb)
target_compile_features(transport-plugin PUBLIC cxx_std_11)

if(CMAKE_COMPILER_IS_GNUCXX)
	if(NOT WIN32)
		add_definitions(-fPIC)
	endif()
endif()

target_link_libraries(transport-plugin transport-protocol JsonCpp::JsonCpp ${CMAKE_THREAD_LIBS_INIT})

if(NOT WIN32)
	target_link_libraries(transport-plugin ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES})
else()
	target_link_libraries(transport-plugin ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES} ws2_32.lib)
endif() 
set_target_properties(transport-plugin PROPERTIES
	VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

install(TARGETS transport-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)

#configure_file(transport.pc.in "${CMAKE_CURRENT_SOURCE_DIR}/transport.pc")
#install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/transport.pc" DESTINATION lib/pkgconfig)
