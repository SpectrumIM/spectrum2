cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC google/protobuf/compiler/*.cc google/protobuf/compiler/*.h google/protobuf/compiler/java/*.cc google/protobuf/compiler/java/*.h google/protobuf/compiler/python/*.cc google/protobuf/compiler/python/*.h google/protobuf/compiler/cpp/*.cc google/protobuf/compiler/cpp/*.h)
include_directories(.)
include_directories(../libprotobuf)
ADD_LIBRARY(libprotoc STATIC ${HEADERS} ${SRC})

INSTALL(TARGETS libprotoc LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT libraries)