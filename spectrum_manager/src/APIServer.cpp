#include "APIServer.h"
#include "methods.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <fstream>
#include <string>
#include <cerrno>

static std::string get_http_var(const struct http_message *hm, const char *name) {
	char data[4096];
	data[0] = '\0';

	mg_get_http_var(&hm->body, name, data, sizeof(data));
	if (data[0] != '\0') {
		return data;
	}

	mg_get_http_var(&hm->query_string, name, data, sizeof(data));
	if (data[0] != '\0') {
		return data;
	}

	return "";
}

APIServer::APIServer(ManagerConfig *config, StorageBackend *storage) {
	m_config = config;
	m_storage = storage;
}

APIServer::~APIServer() {
}

void APIServer::send_json(struct mg_connection *conn, const std::string &json) {
	mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/json\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s",
			(int) json.size(), json.c_str());
}

void APIServer::serve_instances(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
// 	std::string jid = get_http_var(hm, "jid");
// 	if (!jid.empty()) {
// 		serve_instance(conn, hm, jid);
// 		return;
// 	}

	std::vector<std::string> list = show_list(m_config, false);

	std::string json = "{\"error\":0, \"instances\": [";

	BOOST_FOREACH(std::string &instance, list) {
		json += "{";
		json += "\"id\":\"" + instance + "\",";
		json += "\"name\":\"" + instance + "\",";

		std::string status = server->send_command(instance, "status");
		if (status.empty()) {
			status = "Cannot get the instance status.";
		}
		else if (*(status.end() - 1) == '\n') {
			status.erase(status.end() - 1);
		}
		json += "\"status\":\"" + status + "\"";
		json += "},";
	}
	json.erase(json.end() - 1);

	json += "]}";
	send_json(conn, json);
}

void APIServer::handleRequest(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm) {
	if (mg_vcmp(&hm->uri, "/api/v1/instances") == 0) {
		serve_instances(server, sess, conn, hm);
	}
}






