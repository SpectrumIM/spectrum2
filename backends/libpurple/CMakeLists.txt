cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(spectrum2_libpurple_backend ${SRC})

if(NOT WIN32)
target_link_libraries(spectrum2_libpurple_backend ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport-plugin pthread)
else()
include_directories("${CMAKE_SOURCE_DIR}/msvc-deps/protobuf/libprotobuf")
target_link_libraries(spectrum2_libpurple_backend ${PURPLE_LIBRARY} ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport-plugin libprotobuf)
endif()

INSTALL(TARGETS spectrum2_libpurple_backend RUNTIME DESTINATION bin)

