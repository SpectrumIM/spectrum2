cmake_minimum_required(VERSION 2.6)

FILE(GLOB SRC *.c *.cpp)

ADD_EXECUTABLE(spectrum2_template_backend ${SRC})

if (NOT WIN32)
target_link_libraries(spectrum2_template_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else()
target_link_libraries(spectrum2_template_backend transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

#INSTALL(TARGETS spectrum2_template_backend RUNTIME DESTINATION bin)

