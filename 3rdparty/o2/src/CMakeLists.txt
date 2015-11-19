cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(WITH_QT5)
    find_package(Qt5 COMPONENTS Core Network Script REQUIRED)
else(WITH_QT5)
    set(QT_USE_QTNETWORK true)
    set(QT_USE_QTSCRIPT true)
    find_package(Qt4 REQUIRED)
endif(WITH_QT5)
#find_package(QJson REQUIRED)

if (NOT WITH_QT5)
    include( ${QT_USE_FILE} )
endif(NOT WITH_QT5)

set( o2_SRCS
    o2.cpp
    o2reply.cpp
    o2replyserver.cpp
    o2requestor.cpp
    simplecrypt.cpp
    o2settingsstore.cpp
    o2abstractstore.h
)

if(WITH_TWITTER OR WITH_DROPBOX OR WITH_FLICKR)
    set( o2_SRCS
        ${o2_SRCS}
        o1.cpp
        o1requestor.cpp
    )
endif(WITH_TWITTER OR WITH_DROPBOX OR WITH_FLICKR)

if(WITH_TWITTER)
    set( o2_SRCS
        ${o2_SRCS}
        o1twitter.h
        oxtwitter.cpp
    )
endif(WITH_TWITTER)

if(WITH_DROPBOX)
    set( o2_SRCS
        ${o2_SRCS}
        o1dropbox.h
    )
endif(WITH_DROPBOX)

if(WITH_GOOGLE)
    set( o2_SRCS
        ${o2_SRCS}
        o2gft.cpp
    )
endif(WITH_GOOGLE)

if(WITH_FACEBOOK)
    set( o2_SRCS
        ${o2_SRCS}
        o2facebook.cpp
    )
endif(WITH_FACEBOOK)

if(WITH_SKYDRIVE)
    set( o2_SRCS
        ${o2_SRCS}
        o2skydrive.cpp
    )
endif(WITH_SKYDRIVE)

if(WITH_FLICKR)
    set( o2_SRCS
        ${o2_SRCS}
        o1flickr.h
    )
endif(WITH_FLICKR)

if(WITH_HUBIC)
    set( o2_SRCS
        ${o2_SRCS}
        o2hubic.cpp
    )
endif(WITH_HUBIC)

if(NOT WITH_QT5)
    add_definitions(${QT4_DEFINITIONS})
endif(NOT WITH_QT5)

add_library( o2 ${o2_SRCS} )

if(WITH_QT5)
    target_link_libraries( o2 Qt5::Core Qt5::Network Qt5::Script)
else(WITH_QT5)
    target_link_libraries( o2 ${QT_LIBRARIES} )
endif(WITH_QT5)
