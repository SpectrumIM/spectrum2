include_directories (${libtransport_SOURCE_DIR}/backends/twitter/libtwitcurl) 
FILE(GLOB SRC *.cpp libtwitcurl/*.cpp Requests/*.cpp)
add_executable(spectrum2_twitter_backend ${SRC})
#add_executable(parser TwitterResponseParser.cpp test.cpp)
target_link_libraries(spectrum2_twitter_backend curl transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
#target_link_libraries(parser curl transport pthread sqlite3 ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})

INSTALL(TARGETS spectrum2_twitter_backend RUNTIME DESTINATION bin)
