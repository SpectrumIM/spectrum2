cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC google/protobuf/*.cc google/protobuf/*.h google/protobuf/io/*.cc google/protobuf/io/*.h google/protobuf/stubs/*.cc google/protobuf/stubs/*.h google/protobuf/compiler/*.cc google/protobuf/compiler/*.h)

include_directories(.)

ADD_LIBRARY(libprotobuf STATIC ${HEADERS} ${SRC})

INSTALL(TARGETS libprotobuf LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)