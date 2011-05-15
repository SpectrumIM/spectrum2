cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)
FILE(GLOB HEADERS *.h)
QT4_WRAP_CPP(SRC ${HEADERS})
ADD_EXECUTABLE(libircclient-qt_backend ${SRC})

target_link_libraries(libircclient-qt_backend ${IRC_LIBRARY} ${QT_LIBRARIES} transport)

