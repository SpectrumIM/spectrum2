cmake_minimum_required(VERSION 2.6)

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2_swiften_backend ${SRC})

target_link_libraries(spectrum2_swiften_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

INSTALL(TARGETS spectrum2_swiften_backend RUNTIME DESTINATION bin)

