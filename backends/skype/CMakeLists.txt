cmake_minimum_required(VERSION 2.6)
FILE(GLOB SRC *.cpp)

include_directories(/usr/include/dbus-1.0/)
include_directories(/usr/lib/dbus-1.0/include/)
include_directories(/usr/lib64/dbus-1.0/include/)

ADD_EXECUTABLE(spectrum2_skype_backend ${SRC})

target_link_libraries(spectrum2_skype_backend ${GLIB2_LIBRARIES} ${EVENT_LIBRARIES} transport pthread dbus-glib-1 dbus-1 gobject-2.0 transport-plugin)

INSTALL(TARGETS spectrum2_skype_backend RUNTIME DESTINATION bin)

