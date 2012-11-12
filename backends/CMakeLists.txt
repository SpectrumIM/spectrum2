if (PROTOBUF_FOUND)
	if (PURPLE_FOUND)
		ADD_SUBDIRECTORY(libpurple)
	endif()

	if (IRC_FOUND)
		ADD_SUBDIRECTORY(libcommuni)
	endif()

	ADD_SUBDIRECTORY(swiften)

	ADD_SUBDIRECTORY(template)


if (NOT WIN32)
	if(ENABLE_SMSTOOLS3)
		ADD_SUBDIRECTORY(smstools3)
	endif()
	ADD_SUBDIRECTORY(frotz)
 	if(YAHOO2_FOUND)
		ADD_SUBDIRECTORY(libyahoo2)
 	endif()
	if(ENABLE_TWITTER)
		ADD_SUBDIRECTORY(twitter)
	endif()
	if (${LIBDBUSGLIB_FOUND})
		ADD_SUBDIRECTORY(skype)
	endif()
endif()

endif()
