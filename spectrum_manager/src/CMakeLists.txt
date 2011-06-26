cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum_manager ${SRC})

target_link_libraries(spectrum_manager transport)

INSTALL(TARGETS spectrum_manager RUNTIME DESTINATION bin)

INSTALL(FILES
	spectrum_manager.cfg
	DESTINATION /etc/spectrum2
	)
