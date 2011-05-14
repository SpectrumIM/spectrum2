if (PROTOBUF_FOUND)
	ADD_SUBDIRECTORY(libpurple)

	if (IRC_FOUND)
		ADD_SUBDIRECTORY(libircclient-qt)
	endif()

endif()
