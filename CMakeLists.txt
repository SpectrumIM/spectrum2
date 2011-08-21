cmake_minimum_required(VERSION 2.6)
project(libtransport)

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

set(event_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(event)

set(Swiften_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Swiften REQUIRED)

set(Boost_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Boost COMPONENTS date_time system filesystem program_options regex thread signals REQUIRED)

set(Protobuf_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Protobuf REQUIRED)

set(IRCClientQt_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(IRCClientQt)

set(log4cxx_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(log4cxx)

find_package(Doxygen)

INCLUDE(FindQt4)
FIND_PACKAGE(Qt4 COMPONENTS QtCore)

# ADD_DEFINITIONS(${SWIFTEN_CFLAGS})

message("  Supported features")
message("-----------------------")
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

	if(IRC_FOUND)
		ADD_DEFINITIONS(-DIRC_SHARED)
		message("IRC plugin        : yes")
		include_directories(${IRC_INCLUDE_DIR})
		include(${QT_USE_FILE})
	else()
		message("IRC plugin        : no (install libircclient-qt and libprotobuf-dev)")
	endif()

else()
	message("Network plugins   : no (install libprotobuf-dev)")
	message("Libpurple plugin  : no (install libpurple and libprotobuf-dev)")
	message("IRC plugin        : no (install libircclient-qt and libprotobuf-dev)")
endif()

if (LOG4CXX_FOUND)
	message("Logging           : yes")
	include_directories(${LOG4CXX_INCLUDE_DIR})
else()
	message(FATAL_ERROR "Logging           : no (install log4cxx-devel)")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
	ADD_DEFINITIONS(-O0)
	ADD_DEFINITIONS(-ggdb)
	ADD_DEFINITIONS(-DDEBUG)
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


ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(include)
#ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(spectrum)
ADD_SUBDIRECTORY(backends)
#ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(spectrum_manager)

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
