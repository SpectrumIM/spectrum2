cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2 ${SRC})

ADD_DEPENDENCIES(spectrum2 spectrum_libpurple_backend)
ADD_DEPENDENCIES(spectrum2 spectrum_libircclient-qt_backend)

target_link_libraries(spectrum2 transport)

INSTALL(TARGETS spectrum2 RUNTIME DESTINATION bin)

INSTALL(FILES
	sample2.cfg
	RENAME spectrum.cfg.example
	DESTINATION /etc/spectrum2
	)
