include_directories (${libtransport_SOURCE_DIR}/backends/twitter/libtwitcurl) 
FILE(GLOB SRC *.cpp libtwitcurl/*.cpp Requests/*.cpp)
add_executable(spectrum2_twitter_backend ${SRC})

find_package(CURL)

if(CURL_FOUND)
message(STATUS "Using curl ${CURL_VERSION_STRING}: ${CURL_INCLUDE_DIRS} ${CURL_LIBRARIES}")
include_directories (${CURL_INCLUDE_DIRS}) 
target_link_libraries(spectrum2_twitter_backend transport ${CURL_LIBRARIES} ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else()
message(FATAL_ERROR "curl not found")
endif()

INSTALL(TARGETS spectrum2_twitter_backend RUNTIME DESTINATION bin)
