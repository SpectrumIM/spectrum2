cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum_libpurple_backend ${SRC})

target_link_libraries(spectrum_libpurple_backend ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport)

INSTALL(TARGETS spectrum_libpurple_backend RUNTIME DESTINATION bin)

