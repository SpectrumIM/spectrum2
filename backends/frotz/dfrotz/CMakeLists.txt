cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC common/*.c dumb/*.c)

ADD_EXECUTABLE(dfrotz ${SRC})

# target_link_libraries(dfrotz)

INSTALL(TARGETS dfrotz RUNTIME DESTINATION bin)

