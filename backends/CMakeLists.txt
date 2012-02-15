if (PROTOBUF_FOUND)
	if ( PURPLE_LIBRARY AND PURPLE_INCLUDE_DIR )
		ADD_SUBDIRECTORY(libpurple)
	endif()

	if (IRC_FOUND)
		ADD_SUBDIRECTORY(libcommuni)
	endif()

	ADD_SUBDIRECTORY(smstools3)

if (NOT WIN32)
	ADD_SUBDIRECTORY(frotz)
#	ADD_SUBDIRECTORY(skype)
endif()

endif()
