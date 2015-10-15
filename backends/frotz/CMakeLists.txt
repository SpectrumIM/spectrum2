cmake_minimum_required(VERSION 2.6)

ADD_SUBDIRECTORY(dfrotz)

FILE(GLOB SRC *.c *.cpp)

ADD_EXECUTABLE(spectrum2_frotz_backend ${SRC})

target_link_libraries(spectrum2_frotz_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

INSTALL(TARGETS spectrum2_frotz_backend RUNTIME DESTINATION bin)

