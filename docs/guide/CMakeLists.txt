cmake_minimum_required(VERSION 2.4)

macro(add_textile SRC TITLE)
	string(REGEX REPLACE ".textile\$" "" outfileName "${SRC}")
	set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}.html")
	# Custom command to do the processing
	if(${ARGN})
		add_custom_command(OUTPUT "${outfile}"
			COMMAND pandoc -o "${outfile}" "${SRC}" -f textile -t html -s --base-header-level=2 --template=${CMAKE_CURRENT_SOURCE_DIR}/template.html -T "${TITLE}"
			DEPENDS "${SRC}")
	else()
		add_custom_command(OUTPUT "${outfile}"
			COMMAND pandoc -o "${outfile}" "${SRC}" -f textile -t html -s --toc --base-header-level=2 --template=${CMAKE_CURRENT_SOURCE_DIR}/template.html -T "${TITLE}"
			DEPENDS "${SRC}")
	endif()

	# Finally remember the output file for dependencies
	set(outFiles ${outFiles} "${outfile}")
endmacro()

add_textile("index.textile" "Spectrum 2 documentation" 1)
add_textile("about.textile" "Spectrum 2 - What is Spectrum 2?")
add_textile("configure_xmpp_server.textile" "Spectrum 2 - Configure XMPP server")
add_textile("tutorial_gateway_mode.textile" "Spectrum 2 - Spectrum 2 in gateway mode")
add_textile("config_file.textile" "Spectrum 2 - Config File")
add_textile("server_ssl.textile" "Spectrum 2 - Server mode SSL support")
add_textile("mysql.textile" "Spectrum 2 - MySQL Support")
add_textile("postgresql.textile" "Spectrum 2 - PostgreSQL Support")
add_textile("logging.textile" "Spectrum 2 - Logging")
add_textile("backends.textile" "Spectrum 2 - Backends overview")
add_textile("libpurple.textile" "Spectrum 2 - Libpurple backend")
add_textile("libcommuni.textile" "Spectrum 2 - Libcommuni backend")
add_textile("swiften.textile" "Spectrum 2 - Swiften backend")
add_textile("spectrum2_manager.textile" "Spectrum 2 - spectrum2_manager tool")
add_textile("twitter.textile" "Spectrum 2 - Twitter backend")
add_textile("getting_backtrace.textile" "Spectrum 2 - Getting a backtrace")
add_textile("munin.textile" "Spectrum 2 - Munin integration")

add_textile("developer_arch.textile" "Spectrum 2 - Architecture")
add_textile("developer_lowlevel.textile" "Spectrum 2 - Low level backend creation")

# Setup a target to drive the conversion
add_custom_target(guide DEPENDS ${outFiles})
