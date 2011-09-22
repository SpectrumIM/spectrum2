cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)
FILE(GLOB HEADERS *.h)
QT4_WRAP_CPP(SRC ${HEADERS})
ADD_EXECUTABLE(spectrum2_libircclient-qt_backend ${SRC})

target_link_libraries(spectrum2_libircclient-qt_backend ${IRC_LIBRARY} ${QT_LIBRARIES} transport pthread)

INSTALL(TARGETS spectrum2_libircclient-qt_backend RUNTIME DESTINATION bin)

