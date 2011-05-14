cmake_minimum_required(VERSION 2.6)
project(libtransport)

set(CMAKE_MODULE_PATH "cmake_modules")

set(cppunit_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(cppunit)

set(sqlite3_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(sqlite3)

set(purple_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(purple REQUIRED)

set(glib_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(glib REQUIRED)

set(event_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(event)

set(Swiften_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Swiften REQUIRED)

set(Boost_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Boost COMPONENTS date_time system filesystem program_options regex thread signals REQUIRED)

set(Protobuf_DIR "${CMAKE_SOURCE_DIR}/cmake_modules")
find_package(Protobuf REQUIRED)

find_package(Doxygen)

message("  Supported features")
message("-----------------------")
if (SQLITE3_FOUND)
	ADD_DEFINITIONS(-DWITH_SQLITE)
	include_directories(SQLITE3_INCLUDE_DIR)
	message("SQLite3           : yes")
else (SQLITE3_FOUND)
	set(SQLITE3_LIBRARIES "")
	message("SQLite3           : no")
endif (SQLITE3_FOUND)

if (PROTOBUF_FOUND)
	ADD_DEFINITIONS(-DWITH_PROTOBUF)
	include_directories(${PROTOBUF_INCLUDE_DIRS})
	message("Network plugins   : yes")
else()
	message("Network plugins   : no (install Google Protocol Buffers)")
endif()

if(PURPLE_LIBRARY AND PURPLE_INCLUDE_DIR AND PROTOBUF_FOUND)
	message("Libpurple backend : yes")
else()
	message("Libpurple backend : no (install libpurple and Google Protocol Buffers)")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
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
	message("Debug             : no")
endif(CMAKE_BUILD_TYPE MATCHES Debug)


SET(TRANSPORT_VERSION 2.0)
SET(PROJECT_VERSION 2.0)
include_directories(include)

include_directories(${PURPLE_INCLUDE_DIR})
include_directories(${EVENT_INCLUDE_DIRS})
include_directories(${GLIB2_INCLUDE_DIR})
include_directories(${SWIFTEN_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(spectrum)
ADD_SUBDIRECTORY(backends)

if(DOXYGEN_FOUND)
	message("Docs              : yes")
	ADD_SUBDIRECTORY(docs)
else(DOXYGEN_FOUND)
	message("Docs              : no")
endif(DOXYGEN_FOUND)

message("----------------------")
