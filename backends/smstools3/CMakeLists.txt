cmake_minimum_required(VERSION 2.6)

file(GLOB SRC *.c *.cpp)

add_executable(spectrum2_smstools3_backend ${SRC})

target_link_libraries(spectrum2_smstools3_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

install(TARGETS spectrum2_smstools3_backend RUNTIME DESTINATION bin)
