cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum ${SRC})

ADD_DEPENDENCIES(spectrum libpurple_backend)

target_link_libraries(spectrum ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport)

