cmake_minimum_required(VERSION 2.6)
file(GLOB SRC common/*.c dumb/*.c)

add_executable(dfrotz ${SRC})

# target_link_libraries(dfrotz)

install(TARGETS dfrotz RUNTIME DESTINATION bin)

