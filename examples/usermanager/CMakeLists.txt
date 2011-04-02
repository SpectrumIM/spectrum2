FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(transport_usermanager ${SRC})

TARGET_LINK_LIBRARIES(transport_usermanager transport ${SWIFTEN_LIBRARIES})

