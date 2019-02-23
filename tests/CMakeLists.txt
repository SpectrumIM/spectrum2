ADD_SUBDIRECTORY(libtransport)

add_custom_target(test
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output
)
add_custom_target(extended_test
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/start.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output
)

if (PROTOBUF_FOUND)
	if (PURPLE_FOUND)
		ADD_SUBDIRECTORY(libpurple)
		ADD_CUSTOM_TARGET(test_backend_libpurple
			COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libpurple/backend_libpurple_test
		)
		ADD_DEPENDENCIES(test test_backend_libpurple)
		ADD_DEPENDENCIES(extended_test test_backend_libpurple)
	endif()
endif()