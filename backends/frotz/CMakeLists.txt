find_program(FROTZ_EXECUTABLE dfrotz)
if(NOT FROTZ_EXECUTABLE)
	message(FATAL_ERROR "dfrotz not found — install the frotz package")
endif()

file(GLOB SRC *.c *.cpp)

add_executable(spectrum2_frotz_backend ${SRC})
target_compile_features(spectrum2_frotz_backend PUBLIC cxx_std_11)

target_link_libraries(spectrum2_frotz_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

install(TARGETS spectrum2_frotz_backend RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
