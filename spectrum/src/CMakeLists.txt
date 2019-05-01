cmake_minimum_required(VERSION 2.6)

file(GLOB SRC *.cpp)
#include_directories(frontends/xmpp)

add_subdirectory(frontends)

if(WIN32)
	file(GLOB WIN_SRC win32/*.cpp)
	include_directories(win32)
	include_directories("${CMAKE_SOURCE_DIR}/msvc-deps/sqlite3")
	add_executable(spectrum2 ${SRC} ${WIN_SRC})
else()
	add_executable(spectrum2 ${SRC})
endif()

add_dependencies(spectrum2 spectrum2_libpurple_backend)

target_link_libraries(spectrum2 transport spectrum2-xmpp-frontend spectrum2-slack-frontend ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES} ${PROTOBUF_LIBRARY})

if(NOT MSVC AND NOT APPLE)
	# export all symbols (used for loading frontends)
	set(CMAKE_EXE_LINKER_FLAGS "-Wl,-export-dynamic")
endif()

install(TARGETS spectrum2 RUNTIME DESTINATION bin)

install(FILES
	sample2_gateway.cfg
	RENAME spectrum.cfg.example
	DESTINATION /etc/spectrum2/transports
)

install(FILES
	sample2.cfg
	RENAME spectrum_server_mode.cfg.example
	DESTINATION /etc/spectrum2/transports
)

install(FILES
	backend-logging.cfg
	DESTINATION /etc/spectrum2
)

install(FILES
	logging.cfg
	DESTINATION /etc/spectrum2
)

install(FILES
	manager-logging.cfg
	DESTINATION /etc/spectrum2
)
