cmake_minimum_required(VERSION 2.6)

file(GLOB SRC *.cpp *.h)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/include/transport/*.h)

include_directories(${CMAKE_SOURCE_DIR}/spectrum/src/frontends/slack)

if(NOT WIN32)
	add_library(spectrum2-slack-frontend STATIC ${HEADERS} ${SRC} ${EXTRA_SOURCES})
else()
	add_library(spectrum2-slack-frontend STATIC ${HEADERS} ${SRC} ${EXTRA_SOURCES} )
endif()

add_dependencies(spectrum2-slack-frontend transport)

if(CMAKE_COMPILER_IS_GNUCXX)
	if(NOT WIN32)
		add_definitions(-fPIC)
	endif()
endif()

target_link_libraries(spectrum2-slack-frontend transport ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY})

#set_target_properties(transport-plugin PROPERTIES
#      VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
#)

# install(TARGETS transport-plugin LIBRARY DESTINATION ${LIB_INSTALL_DIR} ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries)
