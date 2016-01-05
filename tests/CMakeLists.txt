ADD_SUBDIRECTORY(libtransport)


add_custom_target(test ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output)
add_custom_target(extended_test ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/start.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output)
