cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)
# include_directories(frontends/xmpp)


ADD_SUBDIRECTORY(frontends)

if (WIN32)
FILE(GLOB WIN_SRC win32/*.cpp)
include_directories(win32)
include_directories("${CMAKE_SOURCE_DIR}/msvc-deps/sqlite3")
ADD_EXECUTABLE(spectrum2 ${SRC} ${WIN_SRC})
else()
ADD_EXECUTABLE(spectrum2 ${SRC})
endif()



ADD_DEPENDENCIES(spectrum2 spectrum2_libpurple_backend)

if (WIN32)
target_link_libraries(spectrum2 transport sqlite3 spectrum2-xmpp-frontend spectrum2-slack-frontend ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${PROTOBUF_LIBRARY})
else ()
target_link_libraries(spectrum2 transport spectrum2-xmpp-frontend spectrum2-slack-frontend ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${PROTOBUF_LIBRARY})
endif()

if (NOT MSVC)
# export all symbols (used for loading frontends)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-export-dynamic")
endif()

INSTALL(TARGETS spectrum2 RUNTIME DESTINATION bin)

INSTALL(FILES
	sample2_gateway.cfg
	RENAME spectrum.cfg.example
	DESTINATION /etc/spectrum2/transports
	)

INSTALL(FILES
	sample2.cfg
	RENAME spectrum_server_mode.cfg.example
	DESTINATION /etc/spectrum2/transports
	)

INSTALL(FILES
	backend-logging.cfg
	DESTINATION /etc/spectrum2
	)

INSTALL(FILES
	logging.cfg
	DESTINATION /etc/spectrum2
	)

INSTALL(FILES
	manager-logging.cfg
	DESTINATION /etc/spectrum2
	)

