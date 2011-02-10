FILE(GLOB SRC *.cpp *.h)
FILE(GLOB HEADERS ../../include/transport/*.h)

# SOURCE_GROUP(headers FILES ${HEADERS})

ADD_LIBRARY(transport ${HEADERS} ${SRC})

TARGET_LINK_LIBRARIES(transport -lSwiften -lresolv -lidn -lz -lpthread -lexpat -lidn -lboost_date_time -lboost_system -lboost_filesystem -lboost_program_options -lboost_regex -lboost_thread-mt -lboost_signals -lz -lssl -lcrypto -lexpat -lresolv -lc -lxml2 -export-dynamic)



CONFIGURE_FILE(transport.pc.in "${CMAKE_CURRENT_BINARY_DIR}/transport.pc")
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/transport.pc" DESTINATION lib/pkgconfig)

INSTALL(TARGETS transport LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)
