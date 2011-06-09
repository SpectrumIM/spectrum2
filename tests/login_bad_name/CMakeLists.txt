FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(login_bad_name_test ${SRC})

TARGET_LINK_LIBRARIES(login_bad_name_test transport ${SWIFTEN_LIBRARIES} -lgconf-2 -lgobject-2.0 -lglib-2.0)

