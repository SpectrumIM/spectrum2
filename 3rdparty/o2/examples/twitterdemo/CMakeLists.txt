cmake_minimum_required(VERSION 2.8.11)

project( twitterexample )

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(QT_USE_QTNETWORK true)
set(QT_USE_QTSCRIPT true)

find_package(Qt4 REQUIRED)

include( ${QT_USE_FILE} )
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} "../../src" )

set(fb_SRCS
    main.cpp
    tweeter.cpp
)

add_definitions(${QT4_DEFINITIONS})
add_executable( twitterexample ${fb_SRCS} )

target_link_libraries( twitterexample ${QT_LIBRARIES} o2 )
