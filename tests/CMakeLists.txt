	add_subdirectory(libtransport)
	add_subdirectory(integration)

	if(ENABLE_TESTS)

	  add_custom_target(test
	    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test
	    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output
	  )

	  add_custom_target(extended_test
	    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libtransport/libtransport_test
	    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/integration/run.sh ../libcommuni/irc_test.cfg ${CMAKE_CURRENT_BINARY_DIR}/integration/integration_test localhost.port irc_server
	    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/integration/run.sh ../libcommuni/irc_test2.cfg ${CMAKE_CURRENT_BINARY_DIR}/integration/integration_test
	    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/integration/run.sh ../libpurple_jabber/swiften_test.cfg ${CMAKE_CURRENT_BINARY_DIR}/integration/integration_test localhostxmpp.port jabber
	    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests_output
	  )

	endif()
