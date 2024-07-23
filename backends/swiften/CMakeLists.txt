file(GLOB SRC *.cpp)

add_executable(spectrum2_swiften_backend ${SRC})

target_link_libraries(spectrum2_swiften_backend transport-plugin ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

install(TARGETS spectrum2_swiften_backend RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
