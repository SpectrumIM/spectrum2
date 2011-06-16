cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)
FILE(GLOB HEADERS *.h)
QT4_WRAP_CPP(SRC ${HEADERS})
ADD_EXECUTABLE(spectrum_libircclient-qt_backend ${SRC})

target_link_libraries(spectrum_libircclient-qt_backend ${IRC_LIBRARY} ${QT_LIBRARIES} transport  -lpthread -lssl -lidn -lxml2)

INSTALL(TARGETS spectrum_libircclient-qt_backend RUNTIME DESTINATION bin)

