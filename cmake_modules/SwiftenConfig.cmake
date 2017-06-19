FIND_LIBRARY(SWIFTEN_LIBRARY NAMES Swiften Swiften3 Swiften4 HINTS ../lib)
FIND_PATH(SWIFTEN_INCLUDE_DIR NAMES "Swiften/Swiften.h" PATH_SUFFIXES libSwiften Swiften HINTS ../include)

if( SWIFTEN_LIBRARY AND SWIFTEN_INCLUDE_DIR )
	find_program(SWIFTEN_CONFIG_EXECUTABLE NAMES swiften-config DOC "swiften-config executable" HINTS ../bin)
	set( SWIFTEN_CFLAGS "" )
	if (SWIFTEN_CONFIG_EXECUTABLE)
		# Libs
		execute_process(
			COMMAND ${SWIFTEN_CONFIG_EXECUTABLE} --libs
			OUTPUT_VARIABLE SWIFTEN_LIB)
		string(REGEX REPLACE "[\r\n]"                  " " SWIFTEN_LIB "${SWIFTEN_LIB}")
		string(REGEX REPLACE " +$"                     ""  SWIFTEN_LIB "${SWIFTEN_LIB}")
		set(SWIFTEN_LIBRARY "")
		if (APPLE)
			string(REGEX MATCHALL "-framework [A-Za-z]+" APPLE_FRAMEWORKS "${SWIFTEN_LIB}")
			foreach(framework ${APPLE_FRAMEWORKS})
				list(APPEND SWIFTEN_LIBRARY ${framework} )
			endforeach(framework)
			string(REGEX REPLACE "-framework [A-Za-z]+" "" SWIFTEN_LIB "${SWIFTEN_LIB}")
		endif(APPLE)
		string(REGEX REPLACE " " ";" SWIFTEN_LIB "${SWIFTEN_LIB}")
		foreach(f ${SWIFTEN_LIB})
			STRING(SUBSTRING ${f} 0 2 f_out)
			STRING(COMPARE EQUAL ${f_out} "/L" IS_PATH)
			if(${IS_PATH})
				string(REGEX REPLACE "/LIBPATH:" ""  f_replaced "${f}")
				message("Added link directory: ${f_replaced}")
				link_directories(${f_replaced})
			else()
				list(APPEND SWIFTEN_LIBRARY ${f})
			endif()
		endforeach(f)
		
		# Version
		execute_process(
			COMMAND ${SWIFTEN_CONFIG_EXECUTABLE} --version
			OUTPUT_VARIABLE SWIFTEN_VERSION)
		string(REGEX REPLACE "[\r\n]"                  " " SWIFTEN_VERSION "${SWIFTEN_VERSION}")
		string(REGEX REPLACE " +$"                     ""  SWIFTEN_VERSION "${SWIFTEN_VERSION}")
		string(REGEX REPLACE "swiften-config "          ""  SWIFTEN_VERSION "${SWIFTEN_VERSION}")

		if("${SWIFTEN_VERSION}" STRGREATER "4" AND NOT MSVC)
			message( STATUS "Found Swiften > 4 requesting C++11")
			add_definitions(-std=c++11)
		endif()
		
		set( SWIFTEN_FOUND 1 )
	else()
		message( STATUS "Could NOT find swiften-config" )
	endif()

	if (SWIFTEN_FOUND)
		set( SWIFTEN_INCLUDE_DIR ${SWIFTEN_INCLUDE_DIR} )
		message( STATUS "Found libSwiften: ${SWIFTEN_LIBRARY}, ${SWIFTEN_INCLUDE_DIR}")
	endif()

else( SWIFTEN_LIBRARY AND SWIFTEN_INCLUDE_DIR )
    message( STATUS "Could NOT find libSwiften" )
endif( SWIFTEN_LIBRARY AND SWIFTEN_INCLUDE_DIR )
