cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp *.h)
FILE(GLOB HEADERS ../../include/transport/*.h)

# SOURCE_GROUP(headers FILES ${HEADERS})

ADD_LIBRARY(transport SHARED ${HEADERS} ${SRC})
ADD_DEFINITIONS(-fPIC)

TARGET_LINK_LIBRARIES(transport -lresolv -lidn -lz -lpthread -lexpat -lidn -lz -lssl -lcrypto -lexpat -lresolv -lc -lxml2 ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${SWIFTEN_LIBRARY})

SET_TARGET_PROPERTIES(transport PROPERTIES
      VERSION ${TRANSPORT_VERSION} SOVERSION ${TRANSPORT_VERSION}
)

INSTALL(TARGETS transport LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)

CONFIGURE_FILE(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)
