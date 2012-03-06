cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2_skype_backend ${SRC})

target_link_libraries(spectrum2_skype_backend ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport pthread ${LIBDBUSGLIB_LIBRARIES})

INSTALL(TARGETS spectrum2_skype_backend RUNTIME DESTINATION bin)

