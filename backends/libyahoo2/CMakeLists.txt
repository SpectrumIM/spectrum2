cmake_minimum_required(VERSION 2.6)

FILE(GLOB_RECURSE SRC *.c *.cpp)

ADD_DEFINITIONS(-DHAVE_STDINT_H=1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yahoo)

ADD_EXECUTABLE(spectrum2_libyahoo2_backend ${SRC})

target_link_libraries(spectrum2_libyahoo2_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

INSTALL(TARGETS spectrum2_libyahoo2_backend RUNTIME DESTINATION bin)

