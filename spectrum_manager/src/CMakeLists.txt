cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2_manager ${SRC} ../../src/config.cpp)

target_link_libraries(spectrum2_manager ${SWIFTEN_LIBRARY})

INSTALL(TARGETS spectrum2_manager RUNTIME DESTINATION bin)

INSTALL(FILES
	spectrum_manager.cfg
	DESTINATION /etc/spectrum2
	)
