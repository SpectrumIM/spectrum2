cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2 ${SRC})

ADD_DEPENDENCIES(spectrum2 spectrum2_libpurple_backend)
ADD_DEPENDENCIES(spectrum2 spectrum2_libircclient-qt_backend)

target_link_libraries(spectrum2 transport ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
if (WIN32)
	target_link_libraries(spectrum2 libprotobuf)
endif()

INSTALL(TARGETS spectrum2 RUNTIME DESTINATION bin)

INSTALL(FILES
	sample2.cfg
	RENAME spectrum.cfg.example
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


