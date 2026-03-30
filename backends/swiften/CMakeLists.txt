file(GLOB SRC *.cpp)

add_executable(spectrum2_swiften_backend ${SRC})
target_compile_features(spectrum2_swiften_backend PUBLIC cxx_std_11)


if(NOT WIN32)
	target_link_libraries(spectrum2_swiften_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else()
	target_link_libraries(spectrum2_swiften_backend transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

install(TARGETS spectrum2_swiften_backend RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
