file(GLOB SRC *.cpp)
#include_directories(frontends/xmpp)

add_subdirectory(frontends)

if(WIN32)
	file(GLOB WIN_SRC win32/*.cpp)
	include_directories(win32)
	add_executable(spectrum2 ${SRC} ${WIN_SRC})
else()
	add_executable(spectrum2 ${SRC})
endif()

if (ENABLE_PURPLE AND PURPLE_FOUND)
add_dependencies(spectrum2 spectrum2_libpurple_backend)
endif()

target_link_libraries(spectrum2 transport spectrum2-xmpp-frontend ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${PROTOBUF_LIBRARY})

if(ENABLE_SLACK_FRONTEND)
	target_link_libraries(spectrum2 spectrum2-slack-frontend)
endif()

if(NOT MSVC AND NOT APPLE)
	# export all symbols (used for loading frontends)
	set(CMAKE_EXE_LINKER_FLAGS "-Wl,-export-dynamic")
endif()

install(TARGETS spectrum2 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
	sample2_gateway.cfg
	RENAME spectrum.cfg.example
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/spectrum2/transports
)

install(FILES
	sample2.cfg
	RENAME spectrum_server_mode.cfg.example
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/spectrum2/transports
)

install(FILES
	backend-logging.cfg
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/spectrum2
)

install(FILES
	logging.cfg
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/spectrum2
)

install(FILES
	manager-logging.cfg
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/spectrum2
)
