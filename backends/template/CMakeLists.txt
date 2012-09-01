cmake_minimum_required(VERSION 2.6)

FILE(GLOB SRC *.c *.cpp)

ADD_EXECUTABLE(spectrum2_template_backend ${SRC})

if (CMAKE_COMPILER_IS_GNUCXX)
if (NOT WIN32)
target_link_libraries(spectrum2_template_backend transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else()
target_link_libraries(spectrum2_template_backend transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()
else()
include_directories("${CMAKE_SOURCE_DIR}/msvc-deps/protobuf/libprotobuf")
target_link_libraries(spectrum2_template_backend transport libprotobuf ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

#INSTALL(TARGETS spectrum2_template_backend RUNTIME DESTINATION bin)

