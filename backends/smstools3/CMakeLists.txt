file(GLOB SRC *.c *.cpp)

add_executable(spectrum2_smstools3_backend ${SRC})

target_link_libraries(spectrum2_smstools3_backend transport ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES})

install(TARGETS spectrum2_smstools3_backend RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
