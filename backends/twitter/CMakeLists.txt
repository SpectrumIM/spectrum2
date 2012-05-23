include_directories (${libtransport_SOURCE_DIR}/backends/twitter/libtwitcurl) 
link_directories (${libtransport_BIN_DIR}/backends/twitter/libtwitcurl) 
add_executable(spectrum_twitter_backend main.cpp)
target_link_libraries(spectrum_twitter_backend twitcurl curl)
