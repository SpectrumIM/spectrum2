cmake_minimum_required(VERSION 2.6)

file(GLOB SRC *.cpp)

add_executable(spectrum2_swiften_backend ${SRC})

if(NOT WIN32)
	target_link_libraries(spectrum2_swiften_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else()
	target_link_libraries(spectrum2_swiften_backend transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

install(TARGETS spectrum2_swiften_backend RUNTIME DESTINATION bin)
