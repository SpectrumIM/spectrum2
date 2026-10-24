# transport-protocol: wire protocol for communication between Spectrum2
# frontend and backend plugins, plus shared utilities (Config, Util,
# MemoryUsage). Linked by both libtransport and transport-plugin.

file(GLOB SRC *.cpp)

add_library(transport-protocol SHARED
	${SRC}
	${CMAKE_CURRENT_SOURCE_DIR}/inih/ini.c
	${CMAKE_CURRENT_SOURCE_DIR}/../include/transport/protocol.pb.cc
)

# inih parses the config files: no multi-line values, '#' comments are
# stripped by Config itself, lines of any length.
target_compile_definitions(transport-protocol PRIVATE
	INI_ALLOW_MULTILINE=0
	INI_ALLOW_INLINE_COMMENTS=0
	INI_STOP_ON_FIRST_ERROR=1
	INI_USE_STACK=0
	INI_ALLOW_REALLOC=1
	INI_MAX_LINE=1048576
)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)
add_dependencies(transport-protocol pb)
# Compiled as C++20; public headers stay C++17-compatible because Swiften-based
# consumers (libtransport, XMPP frontend) cannot build as C++20 with Boost 1.78.
target_compile_features(transport-protocol PRIVATE cxx_std_20 INTERFACE cxx_std_17)

if(WIN32)
	target_compile_definitions(transport-protocol PRIVATE TRANSPORT_API=__declspec\(dllexport\))
	target_compile_definitions(transport-protocol INTERFACE TRANSPORT_API=__declspec\(dllimport\))
	target_link_libraries(transport-protocol psapi.lib bcrypt.lib)
else()
	target_compile_definitions(transport-protocol PUBLIC TRANSPORT_API=)
endif()

target_link_libraries(transport-protocol
	${PROTOBUF_LIBRARY}
	${LOG4CXX_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(transport-protocol PROPERTIES
	VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

install(TARGETS transport-protocol LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
