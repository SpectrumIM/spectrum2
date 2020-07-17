cmake_minimum_required(VERSION 2.6)

add_subdirectory(dfrotz)

file(GLOB SRC *.c *.cpp)

add_executable(spectrum2_frotz_backend ${SRC})


target_link_libraries(spectrum2_frotz_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

install(TARGETS spectrum2_frotz_backend RUNTIME DESTINATION bin)
