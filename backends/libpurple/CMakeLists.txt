file(GLOB SRC *.cpp)

add_executable(spectrum2_libpurple_backend ${SRC})
target_compile_features(spectrum2_libpurple_backend PUBLIC cxx_std_11)

target_link_libraries(spectrum2_libpurple_backend transport-plugin ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES})

install(TARGETS spectrum2_libpurple_backend RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
