FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(transport_server_connect ${SRC})

TARGET_LINK_LIBRARIES(transport_server_connect transport)

