cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC google/protobuf/compiler/*.cc google/protobuf/compiler/*.h)

include_directories(.)
include_directories(../libprotobuf)
include_directories(../libprotoc)

ADD_EXECUTABLE(protoc ${HEADERS} ${SRC})

target_link_libraries(protoc libprotoc libprotobuf)

INSTALL(TARGETS protoc RUNTIME DESTINATION bin)