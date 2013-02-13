cmake_minimum_required(VERSION 2.6)

FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2_swiften_raw_backend ${SRC})

IF (NOT WIN32)
target_link_libraries(spectrum2_swiften_raw_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else()
target_link_libraries(spectrum2_swiften_raw_backend transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

INSTALL(TARGETS spectrum2_swiften_raw_backend RUNTIME DESTINATION bin)

