if(ENABLE_TESTS AND NOT WIN32)
	add_executable(integration_test
		test_chat.cpp
	)

	target_link_libraries(integration_test
		transport
		${CPPUNIT_LIBRARY}
		${Boost_LIBRARIES}
	)

	set_target_properties(integration_test PROPERTIES
		COMPILE_DEFINITIONS "LIBTRANSPORT_TEST=1"
	)
endif()
