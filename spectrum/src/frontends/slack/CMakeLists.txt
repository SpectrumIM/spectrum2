cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)
FILE(GLOB HEADERS ${CMAKE_SOURCE_DIR}/include/transport/*.h)

include_directories(${CMAKE_SOURCE_DIR}/spectrum/src/frontends/slack)

if (NOT WIN32)
	ADD_LIBRARY(spectrum2-slack-frontend STATIC ${HEADERS} ${SRC} ${EXTRA_SOURCES})
else()
	ADD_LIBRARY(spectrum2-slack-frontend STATIC ${HEADERS} ${SRC} ${EXTRA_SOURCES} )
endif()
ADD_DEPENDENCIES(spectrum2-slack-frontend transport)

if (CMAKE_COMPILER_IS_GNUCXX)
	if (NOT WIN32)
	ADD_DEFINITIONS(-fPIC)
	endif()
endif()

TARGET_LINK_LIBRARIES(spectrum2-slack-frontend transport ${LOG4CXX_LIBRARIES} ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY})

# SET_TARGET_PROPERTIES(transport-plugin PROPERTIES
#       VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
# )

# INSTALL(TARGETS transport-plugin LIBRARY DESTINATION ${LIB_INSTALL_DIR} ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libraries)

