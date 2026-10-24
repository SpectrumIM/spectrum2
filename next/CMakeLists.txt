# "next": simple raw-protobuf frontend and a small IRC backend.
# Uses transport-protocol for the generated protocol.pb.* (it links protobuf).

add_library(spectrum_next_common STATIC common/wire.cpp common/client_json.cpp)
target_compile_features(spectrum_next_common PUBLIC cxx_std_20)
target_include_directories(spectrum_next_common PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/common
	${CMAKE_SOURCE_DIR}/include/transport)
target_link_libraries(spectrum_next_common transport-protocol JsonCpp::JsonCpp)
if(WIN32)
	target_link_libraries(spectrum_next_common ws2_32.lib)
endif()

add_executable(spectrum_next_frontend frontend/main.cpp)
target_compile_features(spectrum_next_frontend PUBLIC cxx_std_20)
target_link_libraries(spectrum_next_frontend spectrum_next_common)

add_executable(spectrum_next_irc irc/main.cpp)
target_compile_features(spectrum_next_irc PUBLIC cxx_std_20)
target_link_libraries(spectrum_next_irc spectrum_next_common)
