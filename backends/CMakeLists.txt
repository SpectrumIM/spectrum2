if (PROTOBUF_FOUND)
	if ( PURPLE_LIBRARY AND PURPLE_INCLUDE_DIR )
		ADD_SUBDIRECTORY(libpurple)
	endif()

	if (IRC_FOUND)
		ADD_SUBDIRECTORY(libcommuni)
	endif()

	ADD_SUBDIRECTORY(smstools3)

	ADD_SUBDIRECTORY(swiften)

	if(YAHOO2_FOUND)
		ADD_SUBDIRECTORY(libyahoo2)
	endif()

	ADD_SUBDIRECTORY(template)

if (NOT WIN32)
	ADD_SUBDIRECTORY(frotz)
	if (${LIBDBUSGLIB_FOUND})
		ADD_SUBDIRECTORY(skype)
	endif()
endif()

endif()
