cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum ${SRC})

ADD_DEPENDENCIES(spectrum libpurple_backend)
ADD_DEPENDENCIES(spectrum libircclient-qt_backend)

target_link_libraries(spectrum transport)

