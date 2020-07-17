cmake_minimum_required(VERSION 2.6)

file(GLOB SRC *.cpp *.c)

add_executable(spectrum2_manager ${SRC})
add_dependencies(spectrum2_manager pb)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)

target_link_libraries(spectrum2_manager transport ${SWIFTEN_LIBRARY} ${PROTOBUF_LIBRARIES})

if(${OPENSSL_FOUND})
	add_definitions(-DMG_ENABLE_SSL)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(spectrum2_manager ${OPENSSL_LIBRARIES})
endif()

if(APPLE)
	target_link_libraries(spectrum2_manager transport ${APPLE_FRAMEWORKS})
endif()

install(TARGETS spectrum2_manager RUNTIME DESTINATION bin)

# if(NOT EXISTS "/etc/spectrum2/spectrum_manager.cfg")
# install(FILES
# 	spectrum_manager.cfg
# 	DESTINATION /etc/spectrum2
# 	)
# endif()

install(CODE "
if(NOT EXISTS \"$ENV{DESTDIR}/etc/spectrum2/spectrum_manager.cfg\")
file(INSTALL DESTINATION \"/etc/spectrum2\" TYPE FILES \"${CMAKE_CURRENT_SOURCE_DIR}/spectrum_manager.cfg\")
endif()
")

install(DIRECTORY
	html
	DESTINATION /var/lib/spectrum2_manager
)
