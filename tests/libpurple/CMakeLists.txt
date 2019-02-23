cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)

include_directories(../../backends/libpurple)

if (CPPUNIT_FOUND)
	FILE(GLOB SRC_TEST_BACKEND_LIBPURPLE *.cpp)
	FILE(GLOB SRC_BACKEND_LIBPURPLE ../../backends/libpurple/loopback.cpp)

	ADD_EXECUTABLE(backend_libpurple_test ${SRC_TEST_BACKEND_LIBPURPLE} ${SRC_BACKEND_LIBPURPLE})
	set_target_properties(backend_libpurple_test PROPERTIES COMPILE_DEFINITIONS BACKEND_LIBPURPLE_TEST=1)

	target_link_libraries(backend_libpurple_test ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${LOG4CXX_LIBRARIES})
	target_link_libraries(backend_libpurple_test ${CPPUNIT_LIBRARY} ${Boost_LIBRARIES})
endif()
