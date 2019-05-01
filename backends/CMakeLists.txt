if(PROTOBUF_FOUND)
	add_subdirectory(template)

	if(PURPLE_FOUND)
		add_subdirectory(libpurple)
	endif()

	if(IRC_FOUND)
		add_subdirectory(libcommuni)
	endif()

	if(ENABLE_XMPP)
		add_subdirectory(swiften)
	endif()

	if(ENABLE_TWITTER)
		add_subdirectory(twitter)
	endif()

	if(NOT WIN32)
		if(ENABLE_SMSTOOLS3)
			add_subdirectory(smstools3)
		endif()
		if(ENABLE_FROTZ)
			add_subdirectory(frotz)
		endif()
	endif()
endif()
