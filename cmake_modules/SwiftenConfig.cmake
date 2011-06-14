FIND_LIBRARY(SWIFTEN_LIBRARY NAMES Swiften)
FIND_PATH(SWIFTEN_INCLUDE_DIR NAMES "Swiften.h" PATH_SUFFIXES libSwiften Swiften )

if( SWIFTEN_LIBRARY AND SWIFTEN_INCLUDE_DIR )
	find_program(SWIFTEN_CONFIG_EXECUTABLE NAMES swiften-config DOC "swiften-config executable")
	set( SWIFTEN_CFLAGS "" )
	if (SWIFTEN_CONFIG_EXECUTABLE)
		execute_process(
			COMMAND swiften-config --libs
			OUTPUT_VARIABLE SWIFTEN_LIBRARY)
		execute_process(
			COMMAND swiften-config --cflags
			OUTPUT_VARIABLE SWIFTEN_CFLAGS)
		string(REGEX REPLACE "[\r\n]"                  " " SWIFTEN_LIBRARY "${SWIFTEN_LIBRARY}")
		string(REGEX REPLACE " +$"                     ""  SWIFTEN_LIBRARY "${SWIFTEN_LIBRARY}")
	endif()

	set( SWIFTEN_INCLUDE_DIR ${SWIFTEN_INCLUDE_DIR}/.. )
	message( STATUS "Found libSwiften: ${SWIFTEN_LIBRARY}, ${SWIFTEN_INCLUDE_DIR}")
	set( SWIFTEN_FOUND 1 )
else( SWIFTEN_LIBRARY AND SWIFTEN_INCLUDE_DIR )
    message( FATAL_ERROR "Could NOT find libSwiften" )
endif( SWIFTEN_LIBRARY AND SWIFTEN_INCLUDE_DIR )
