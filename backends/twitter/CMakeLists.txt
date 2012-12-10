include_directories (${libtransport_SOURCE_DIR}/backends/twitter/libtwitcurl) 
FILE(GLOB SRC *.cpp libtwitcurl/*.cpp Requests/*.cpp)
add_executable(spectrum2_twitter_backend ${SRC})

if (NOT WIN32)
target_link_libraries(spectrum2_twitter_backend curl transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
else ()
target_link_libraries(spectrum2_twitter_backend libcurl_imp transport ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
endif()

INSTALL(TARGETS spectrum2_twitter_backend RUNTIME DESTINATION bin)
