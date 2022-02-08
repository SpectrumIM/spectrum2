file(GLOB SRC *.cpp)

add_executable(spectrum2_libpurple_backend ${SRC})

if(MSVC)
	target_link_libraries(spectrum2_libpurple_backend transport ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${LIBXML2_LIBRARIES} ${EVENT_LIBRARIES} transport-plugin)
else()
	if(NOT WIN32)
		target_link_libraries(spectrum2_libpurple_backend transport ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport-plugin pthread)
	else()
		target_link_libraries(spectrum2_libpurple_backend transport ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport-plugin)
	endif()
endif()

install(TARGETS spectrum2_libpurple_backend RUNTIME DESTINATION bin)
