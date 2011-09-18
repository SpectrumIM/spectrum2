cmake_minimum_required(VERSION 2.6)

ADD_SUBDIRECTORY(dfrotz)

FILE(GLOB SRC *.c *.cpp)

ADD_EXECUTABLE(spectrum_frotz_backend ${SRC})

target_link_libraries(spectrum_frotz_backend transport pthread)

INSTALL(TARGETS spectrum_frotz_backend RUNTIME DESTINATION bin)

