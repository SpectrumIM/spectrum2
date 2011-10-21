if (PROTOBUF_FOUND)
	if ( PURPLE_LIBRARY AND PURPLE_INCLUDE_DIR )
		ADD_SUBDIRECTORY(libpurple)
	endif()

	if (IRC_FOUND)
		ADD_SUBDIRECTORY(libircclient-qt)
	endif()

if (NOT WIN32)
	ADD_SUBDIRECTORY(frotz)
endif()

endif()
