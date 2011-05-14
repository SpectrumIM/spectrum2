cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

ADD_EXECUTABLE(libircclient-qt_backend ${SRC})

target_link_libraries(libircclient-qt_backend ${IRC_LIBRARIES} ${QT_LIBRARIES} transport)

