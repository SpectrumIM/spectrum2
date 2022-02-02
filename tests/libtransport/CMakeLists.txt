file(GLOB SRC *.cpp *.h)
file(GLOB_RECURSE SWIFTEN_SRC ../include/Swiften/*.cpp)

# Build without openssl on msvc
if(NOT MSVC)
	if(APPLE)
		string(REGEX REPLACE "[^;]+;?/Schannel/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
		string(REGEX REPLACE "[^;]+;?/OpenSSL/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
	else()
		string(REGEX REPLACE "[^;]+;?/Schannel/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
		string(REGEX REPLACE "[^;]+;?/SecureTransport/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}") 
	endif()
else()
	string(REGEX REPLACE "[^;]+;?/OpenSSL/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}") 
	string(REGEX REPLACE "[^;]+;?/SecureTransport/[^;]+;?" "" SWIFTEN_SRC "${SWIFTEN_SRC}")
endif()

file(GLOB HEADERS ../../include/transport/*.h)
include_directories(../../spectrum/src/frontends/xmpp/)
include_directories(../../spectrum/src/frontends/slack/)

if(CPPUNIT_FOUND)
	file(GLOB SRC_TEST *.cpp)
	file(GLOB SRC_TEST_FRONTEND_XMPP ../../spectrum/src/frontends/xmpp/*.cpp)
	file(GLOB SRC_TEST_FRONTEND_SLACK ../../spectrum/src/frontends/slack/*.cpp)

	add_executable(libtransport_test ${SRC_TEST} ${SRC_TEST_FRONTEND_XMPP} ${SRC_TEST_FRONTEND_SLACK})
	set_target_properties(libtransport_test PROPERTIES COMPILE_DEFINITIONS LIBTRANSPORT_TEST=1)
	

	target_link_libraries(libtransport_test transport ${CPPUNIT_LIBRARY} ${Boost_LIBRARIES})
endif()
