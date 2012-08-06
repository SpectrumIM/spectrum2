cmake_minimum_required(VERSION 2.6)
project(libtransport)

if(NOT LIB_INSTALL_DIR)
	set(LIB_INSTALL_DIR "lib")
endif()

set(CMAKE_MODULE_PATH "cmake_modules")

set(cppunit_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(cppunit)

set(sqlite3_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(sqlite3)

set(mysql_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(mysql)

set(purple_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(purple)

set(glib_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(glib)

if (NOT WIN32)
	set(popt_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
	find_package(popt)
endif()

set(event_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(event)

set(Swiften_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Swiften)

set(openssl_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(openssl)

set(Boost_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
if (WIN32)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
endif()
find_package(Boost COMPONENTS program_options date_time system filesystem regex  signals REQUIRED)
message( STATUS "Found Boost: ${Boost_LIBRARIES}, ${Boost_INCLUDE_DIR}")

set(Protobuf_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Protobuf REQUIRED)

set(Communi_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Communi)

set(log4cxx_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(log4cxx)

set(event_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(event)

set(pqxx_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(pqxx)

set(dbus_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(dbus)

set(yahoo2_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(yahoo2)

find_package(Doxygen)

INCLUDE(FindQt4)
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtNetwork)

# ADD_DEFINITIONS(${SWIFTEN_CFLAGS})
ADD_DEFINITIONS(-DSUPPORT_LEGACY_CAPS)
ADD_DEFINITIONS(-DBOOST_FILESYSTEM_VERSION=2)

message("  Supported features")
message("-----------------------")

if (SPECTRUM_VERSION)
	ADD_DEFINITIONS(-DSPECTRUM_VERSION="${SPECTRUM_VERSION}")
else (SPECTRUM_VERSION)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
		execute_process(COMMAND git "--git-dir=${CMAKE_CURRENT_SOURCE_DIR}/.git" rev-parse --short HEAD
						OUTPUT_VARIABLE GIT_REVISION
						OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		set(SPECTRUM_VERSION 2.0.0-beta-git-${GIT_REVISION})
		ADD_DEFINITIONS(-DSPECTRUM_VERSION="${SPECTRUM_VERSION}")
	else (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
		set(SPECTRUM_VERSION 2.0.0-alpha)
		ADD_DEFINITIONS(-DSPECTRUM_VERSION="${SPECTRUM_VERSION}")
	endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
endif (SPECTRUM_VERSION)

message("Version           : " ${SPECTRUM_VERSION})

if (SQLITE3_FOUND)
	ADD_DEFINITIONS(-DWITH_SQLITE)
	include_directories(${SQLITE3_INCLUDE_DIR})
	message("SQLite3           : yes")
else (SQLITE3_FOUND)
	set(SQLITE3_LIBRARIES "")
	message("SQLite3           : no")
endif (SQLITE3_FOUND)

if (MYSQL_FOUND)
	ADD_DEFINITIONS(-DWITH_MYSQL)
	include_directories(${MYSQL_INCLUDE_DIR})
	message("MySQL             : yes")
else (MYSQL_FOUND)
	set(MYSQL_LIBRARIES "")
	message("MySQL             : no (install mysql-devel)")
endif (MYSQL_FOUND)

if (PQXX_FOUND)
	ADD_DEFINITIONS(-DWITH_PQXX)
	include_directories(${PQXX_INCLUDE_DIR})
	message("PostgreSQL        : yes")
else (PQXX_FOUND)
	set(PQXX_LIBRARY "")
	set(PQ_LIBRARY "")
	message("PostgreSQL        : no (install libpqxx-devel)")
endif (PQXX_FOUND)

if (PROTOBUF_FOUND)
	ADD_DEFINITIONS(-DWITH_PROTOBUF)
	include_directories(${PROTOBUF_INCLUDE_DIRS})
	message("Network plugins   : yes")

	if(PURPLE_LIBRARY AND PURPLE_INCLUDE_DIR)
		message("Libpurple plugin  : yes")
		include_directories(${PURPLE_INCLUDE_DIR})
		include_directories(${GLIB2_INCLUDE_DIR})
	else()
		message("Libpurple plugin  : no (install libpurple)")
	endif()

	if (HAVE_EVENT)
		ADD_DEFINITIONS(-DWITH_LIBEVENT)
		include_directories(${EVENT_INCLUDE_DIRS})
		message("  libev eventloop : yes")
	else()
		message("  libev eventloop : no (install libev-devel)")
	endif()

	if(IRC_FOUND)
		ADD_DEFINITIONS(-DCOMMUNI_SHARED)
		message("IRC plugin        : yes")
		include_directories(${QT_QTNETWORK_INCLUDE_DIR})
		include_directories(${IRC_INCLUDE_DIR})
		include(${QT_USE_FILE})
	else()
		message("IRC plugin        : no (install libCommuni and libprotobuf-dev)")
	endif()

	message("Frotz plugin      : yes")
	message("SMSTools3 plugin  : yes")

	if(${LIBDBUSGLIB_FOUND})
		message("Skype plugin      : yes")
		include_directories(${LIBDBUSGLIB_INCLUDE_DIRS})
	else()
		message("Skype plugin      : no (install dbus-glib-devel)")
	endif()

#	We have our own copy now...
# 	if(YAHOO2_FOUND)
		message("Libyahoo2 plugin  : yes")
# 		include_directories(${YAHOO2_INCLUDE_DIR})
# 	else()
# 		message("Libyahoo2 plugin  : no (install libyahoo2-devel)")
# 	endif()

else()
	message("Network plugins   : no (install libprotobuf-dev)")
	message("Libpurple plugin  : no (install libpurple and libprotobuf-dev)")
	message("IRC plugin        : no (install libircclient-qt and libprotobuf-dev)")
	message("Frotz plugin      : no (install libprotobuf-dev)")
	message("SMSTools3 plugin  : no (install libprotobuf-dev)")
endif()

if (LOG4CXX_FOUND)
	message("Log4cxx           : yes")
	include_directories(${LOG4CXX_INCLUDE_DIR})
	ADD_DEFINITIONS(-DWITH_LOG4CXX)
else()
	set(LOG4CXX_LIBRARIES "")
	message("Log4cxx           : no (install log4cxx-devel)")
endif()

if (WIN32)
ADD_DEFINITIONS(-D_WIN32_WINNT=0x501)
ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
if (CMAKE_COMPILER_IS_GNUCXX)
	ADD_DEFINITIONS(-O0)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-Wall)
	ADD_DEFINITIONS(-W)
	ADD_DEFINITIONS(-Wcast-align)
	ADD_DEFINITIONS(-Wextra -Wno-sign-compare -Wno-unused-parameter)
	ADD_DEFINITIONS(-Winit-self)
	ADD_DEFINITIONS(-Wmissing-declarations)
	ADD_DEFINITIONS(-Wpointer-arith)
	ADD_DEFINITIONS(-Wreorder)
	ADD_DEFINITIONS(-Woverloaded-virtual)
	ADD_DEFINITIONS(-Wsign-promo)
	ADD_DEFINITIONS(-Wundef -Wunused)
endif()
	ADD_DEFINITIONS(-DDEBUG)
	message("Debug             : yes")
else(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Debug             : no (run \"cmake . -DCMAKE_BUILD_TYPE=Debug\")")
endif(CMAKE_BUILD_TYPE MATCHES Debug)


SET(TRANSPORT_VERSION 2.0)
SET(PROJECT_VERSION 2.0)
include_directories(include)


include_directories(${EVENT_INCLUDE_DIRS})
include_directories(${SWIFTEN_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(plugin)
ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(spectrum)
ADD_SUBDIRECTORY(backends)
if (NOT WIN32)
	ADD_SUBDIRECTORY(spectrum_manager)
#	ADD_SUBDIRECTORY(spectrum2_send_message)
endif()

if (CPPUNIT_FOUND)
	message("tests             : yes")
	include_directories(${CPPUNIT_INCLUDE_DIR})
else()
	message("tests             : no (install CPPUnit)")
endif()

if(DOXYGEN_FOUND)
	message("Docs              : yes")
	ADD_SUBDIRECTORY(docs)
else(DOXYGEN_FOUND)
	message("Docs              : no (install doxygen)")
endif(DOXYGEN_FOUND)

message("----------------------")
