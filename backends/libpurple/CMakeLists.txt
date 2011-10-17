cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2_libpurple_backend ${SRC})

target_link_libraries(spectrum2_libpurple_backend ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport-plugin pthread)

INSTALL(TARGETS spectrum2_libpurple_backend RUNTIME DESTINATION bin)

