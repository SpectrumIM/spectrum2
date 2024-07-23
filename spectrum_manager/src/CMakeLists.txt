file(GLOB SRC *.cpp *.c)

add_executable(spectrum2_manager ${SRC})
add_dependencies(spectrum2_manager pb)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../../include/transport/protocol.pb.cc PROPERTIES GENERATED 1)

target_include_directories(spectrum2_manager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(spectrum2_manager transport ${SWIFTEN_LIBRARY} ${PROTOBUF_LIBRARIES})
target_compile_features(spectrum2_manager PUBLIC cxx_std_11)

if (ENABLE_WEBUI)
	add_definitions(-DENABLE_WEBUI)
	file(GLOB SERVER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/server/*.c)
	target_sources(spectrum2_manager PRIVATE ${SERVER_SRC})
	if(${OPENSSL_FOUND})
		add_definitions(-DMG_ENABLE_SSL)
	endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(spectrum2_manager ${OPENSSL_LIBRARIES})
endif()

if(APPLE)
	target_link_libraries(spectrum2_manager transport ${APPLE_FRAMEWORKS})
endif()

install(TARGETS spectrum2_manager RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# if(NOT EXISTS "/etc/spectrum2/spectrum_manager.cfg")
# install(FILES
# 	spectrum_manager.cfg
# 	DESTINATION /etc/spectrum2
# 	)
# endif()

install(CODE "
if(NOT EXISTS \"$ENV{DESTDIR}/${CMAKE_INSTALL_SYSCONFDIR}/spectrum2/spectrum_manager.cfg\")
file(INSTALL DESTINATION \"${CMAKE_INSTALL_SYSCONFDIR}/spectrum2\" TYPE FILES \"${CMAKE_CURRENT_SOURCE_DIR}/spectrum_manager.cfg\")
endif()
")

if (ENABLE_WEBUI)
	install(DIRECTORY
		html
		DESTINATION ${CMAKE_INSTALL_LOCALSTATEDIR}/lib/spectrum2_manager
	)
endif()
