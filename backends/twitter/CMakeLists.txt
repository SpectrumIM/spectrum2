include_directories (${libtransport_SOURCE_DIR}/backends/twitter/libtwitcurl) 
FILE(GLOB SRC *.cpp libtwitcurl/*.cpp)
add_executable(spectrum_twitter_backend ${SRC})
target_link_libraries(spectrum_twitter_backend curl transport pthread ${Boost_LIBRARIES} ${SWIFTEN_LIBRARY} ${LOG4CXX_LIBRARIES})
