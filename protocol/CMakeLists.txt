# transport-protocol: wire protocol for communication between Spectrum2
# frontend and backend plugins, plus shared utilities (Config, Util,
# MemoryUsage). Linked by both libtransport and transport-plugin.

file(GLOB SRC *.cpp)

add_library(transport-protocol SHARED
	${SRC}
	${CMAKE_CURRENT_SOURCE_DIR}/../include/transport/protocol.pb.cc
)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)
add_dependencies(transport-protocol pb)
target_compile_features(transport-protocol PUBLIC cxx_std_11)

if(WIN32)
	target_compile_definitions(transport-protocol PRIVATE TRANSPORT_API=__declspec\(dllexport\))
	target_compile_definitions(transport-protocol INTERFACE TRANSPORT_API=__declspec\(dllimport\))
	target_link_libraries(transport-protocol psapi.lib bcrypt.lib)
else()
	target_compile_definitions(transport-protocol PUBLIC TRANSPORT_API=)
endif()

target_link_libraries(transport-protocol
	${PROTOBUF_LIBRARY}
	${Boost_LIBRARIES}
	${LOG4CXX_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(transport-protocol PROPERTIES
	VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

install(TARGETS transport-protocol LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
