find_package(Qt4)
include( ${QT_USE_FILE} )

FIND_LIBRARY(IRC_LIBRARY NAMES IrcCore PATHS ${QT_LIBRARY_DIR})
FIND_PATH(IRC_INCLUDE_DIR NAMES "IrcCore/ircglobal.h" PATHS ${QT_INCLUDE_DIR} PATH_SUFFIXES Communi)

# message( STATUS ${IRC_LIBRARY})
if( IRC_LIBRARY AND IRC_INCLUDE_DIR )
	set(IRC_INCLUDE_DIR ${IRC_INCLUDE_DIR}/IrcCore ${IRC_INCLUDE_DIR}/IrcUtil ${IRC_INCLUDE_DIR}/IrcModel)
	message( STATUS "Found libCommuni ${IRC_LIBRARY}, ${IRC_INCLUDE_DIR}")
	set( IRC_FOUND 1 )
else()
	message( STATUS "Could NOT find libCommuni" )
endif()
