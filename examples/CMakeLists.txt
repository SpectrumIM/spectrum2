ADD_SUBDIRECTORY(server_connect)
ADD_SUBDIRECTORY(usermanager)

if (PROTOBUF_FOUND)
	ADD_SUBDIRECTORY(external_network_plugin)
endif()
