macro(ADD_TEXTILE SRC TITLE)
	MESSAGE(STATUS "Process file: ${SRC}")
	STRING(REGEX REPLACE ".textile\$" "" outfileName "${SRC}")
	SET(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}.html")
	# Custom command to do the processing
	message(STATUS "FILES" ${outfile})
	if(${ARGN})
		ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
			COMMAND pandoc -o "${outfile}" "${SRC}" -f textile -t html -s --base-header-level=2 --template=template.html -T "${TITLE}"
			DEPENDS "${SRC}")
	else()
		ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
			COMMAND pandoc -o "${outfile}" "${SRC}" -f textile -t html -s --toc --base-header-level=2 --template=template.html -T "${TITLE}"
			DEPENDS "${SRC}")
	endif()

	# Finally remember the output file for dependencies
	SET(outFiles ${outFiles} "${outfile}")
endmacro()


ADD_TEXTILE("index.textile" "Spectrum 2 documentation" 1)
ADD_TEXTILE("config_file.textile" "Spectrum 2 - Config File")
ADD_TEXTILE("server_ssl.textile" "Spectrum 2 - Server mode SSL support")
ADD_TEXTILE("mysql.textile" "Spectrum 2 - MySQL Support")
ADD_TEXTILE("postgresql.textile" "Spectrum 2 - PostgreSQL Support")
ADD_TEXTILE("logging.textile" "Spectrum 2 - Logging")


# Setup a target to drive the conversion
ADD_CUSTOM_TARGET(guide ALL DEPENDS ${outFiles})

