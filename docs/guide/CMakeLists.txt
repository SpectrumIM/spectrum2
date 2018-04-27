cmake_minimum_required(VERSION 2.4)

macro(ADD_TEXTILE SRC TITLE)
	STRING(REGEX REPLACE ".textile\$" "" outfileName "${SRC}")
	SET(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}.html")
	# Custom command to do the processing
	if(${ARGN})
		ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
			COMMAND pandoc -o "${outfile}" "${SRC}" -f textile -t html -s --base-header-level=2 --template=${CMAKE_CURRENT_SOURCE_DIR}/template.html -T "${TITLE}"
			DEPENDS "${SRC}")
	else()
		ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
			COMMAND pandoc -o "${outfile}" "${SRC}" -f textile -t html -s --toc --base-header-level=2 --template=${CMAKE_CURRENT_SOURCE_DIR}/template.html -T "${TITLE}"
			DEPENDS "${SRC}")
	endif()

	# Finally remember the output file for dependencies
	SET(outFiles ${outFiles} "${outfile}")
endmacro()


ADD_TEXTILE("index.textile" "Spectrum 2 documentation" 1)
ADD_TEXTILE("about.textile" "Spectrum 2 - What is Spectrum 2?")
ADD_TEXTILE("configure_xmpp_server.textile" "Spectrum 2 - Configure XMPP server")
ADD_TEXTILE("tutorial_gateway_mode.textile" "Spectrum 2 - Spectrum 2 in gateway mode")
ADD_TEXTILE("config_file.textile" "Spectrum 2 - Config File")
ADD_TEXTILE("server_ssl.textile" "Spectrum 2 - Server mode SSL support")
ADD_TEXTILE("mysql.textile" "Spectrum 2 - MySQL Support")
ADD_TEXTILE("postgresql.textile" "Spectrum 2 - PostgreSQL Support")
ADD_TEXTILE("logging.textile" "Spectrum 2 - Logging")
ADD_TEXTILE("backends.textile" "Spectrum 2 - Backends overview")
ADD_TEXTILE("libpurple.textile" "Spectrum 2 - Libpurple backend")
ADD_TEXTILE("libcommuni.textile" "Spectrum 2 - Libcommuni backend")
ADD_TEXTILE("swiften.textile" "Spectrum 2 - Swiften backend")
ADD_TEXTILE("spectrum2_manager.textile" "Spectrum 2 - spectrum2_manager tool")
ADD_TEXTILE("twitter.textile" "Spectrum 2 - Twitter backend")
ADD_TEXTILE("getting_backtrace.textile" "Spectrum 2 - Getting a backtrace")
ADD_TEXTILE("munin.textile" "Spectrum 2 - Munin integration")

ADD_TEXTILE("developer_arch.textile" "Spectrum 2 - Architecture")
ADD_TEXTILE("developer_lowlevel.textile" "Spectrum 2 - Low level backend creation")


# Setup a target to drive the conversion
ADD_CUSTOM_TARGET(guide DEPENDS ${outFiles})

