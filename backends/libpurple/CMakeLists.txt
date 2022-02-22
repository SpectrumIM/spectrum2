file(GLOB SRC *.cpp)

add_executable(spectrum2_libpurple_backend ${SRC})

target_link_libraries(spectrum2_libpurple_backend transport-plugin ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES})

install(TARGETS spectrum2_libpurple_backend RUNTIME DESTINATION bin)
