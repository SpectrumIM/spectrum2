cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)
FILE(GLOB HEADERS *.h)
QT4_WRAP_CPP(SRC ${HEADERS} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
ADD_EXECUTABLE(spectrum2_libcommuni_backend ${SRC})

if (NOT WIN32)
	target_link_libraries(spectrum2_libcommuni_backend ${IRC_LIBRARY} ${QT_LIBRARIES} transport pthread)
else ()
	target_link_libraries(spectrum2_libcommuni_backend ${IRC_LIBRARY} ${QT_LIBRARIES} transport)
endif()
INSTALL(TARGETS spectrum2_libcommuni_backend RUNTIME DESTINATION bin)

