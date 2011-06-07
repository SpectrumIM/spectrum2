FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(login_test ${SRC})

TARGET_LINK_LIBRARIES(login_test transport ${SWIFTEN_LIBRARIES})

