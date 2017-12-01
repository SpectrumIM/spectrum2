cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)
FILE(GLOB HEADERS *.h)
QT4_WRAP_CPP(SRC ${HEADERS} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
add_definitions(-DQT_NO_KEYWORDS)
ADD_EXECUTABLE(spectrum2_libcommuni_backend ${SRC})

if (NOT WIN32)
	target_link_libraries(spectrum2_libcommuni_backend ${IRC_LIBRARY} Qt4::QtNetwork Qt4::QtCore transport pthread)
else ()
	target_link_libraries(spectrum2_libcommuni_backend ${IRC_LIBRARY} Qt4::QtNetwork Qt4::QtCore transport)
endif()
INSTALL(TARGETS spectrum2_libcommuni_backend RUNTIME DESTINATION bin)

