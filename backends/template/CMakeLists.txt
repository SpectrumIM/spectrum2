file(GLOB SRC *.c *.cpp)

add_executable(spectrum2_template_backend ${SRC})

if(CMAKE_COMPILER_IS_GNUCXX)
	if(NOT WIN32)
		target_link_libraries(spectrum2_template_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
	else()
		target_link_libraries(spectrum2_template_backend transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
	endif()
else()
	target_link_libraries(spectrum2_template_backend transport ${PROTOBUF_LIBRARY} ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

#install(TARGETS spectrum2_template_backend RUNTIME DESTINATION bin)
