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

#define ALLOW_ONLY_ADMIN() 	if (!session->admin) { \
		std::string _json = "{\"error\":1, \"message\": \"Only administrators can do this API call.\"}"; \
		send_json(conn, _json); \
		return; \
	}

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

static int has_prefix(const struct mg_str *uri, const char *prefix) {
	size_t prefix_len = strlen(prefix);
	return uri->len >= prefix_len && memcmp(uri->p, prefix, prefix_len) == 0;
}

APIServer::APIServer(ManagerConfig *config, StorageBackend *storage) {
	m_config = config;
	m_storage = storage;
}

APIServer::~APIServer() {
}

std::string &APIServer::safe_arg(std::string &arg) {
	boost::replace_all(arg, "\n", "");
	boost::replace_all(arg, "\"", "'");
	return arg;
}

void APIServer::send_json(struct mg_connection *conn, const std::string &json) {
	std::cout << "Sending JSON:\n";
	std::cout << json << "\n";
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
		json += "\"status\":\"" + safe_arg(status) + "\",";

		bool running = true;
		if (status.find("Running") == std::string::npos) {
			running = false;
		}
		json += "\"running\":" + (running ? std::string("1") : std::string("0"));
		json += "},";
	}
	json.erase(json.end() - 1);

	json += "]}";
	send_json(conn, json);
}

void APIServer::serve_instances_start(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	ALLOW_ONLY_ADMIN();

	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	start_instances(m_config, instance);
	std::string response = get_response();
	std::string error = response.find("OK") == std::string::npos ? "1" : "0";
	std::string json = "{\"error\":" + error + ", \"message\": \"" + safe_arg(response) + "\"}";

	// TODO: So far it needs some time to reload Spectrum 2, so just sleep here.
	sleep(1);

	send_json(conn, json);
}

void APIServer::serve_instances_stop(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	ALLOW_ONLY_ADMIN();

	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	stop_instances(m_config, instance);
	std::string response = get_response();
	std::string error = response.find("OK") == std::string::npos ? "1" : "0";
	std::string json = "{\"error\":" + error + ", \"message\": \"" + safe_arg(response) + "\"}"; \
	send_json(conn, json);
}

void APIServer::handleRequest(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm) {
	if (has_prefix(&hm->uri, "/api/v1/instances/start/")) {
		serve_instances_start(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/stop/")) {
		serve_instances_stop(server, sess, conn, hm);
	}
	else if (mg_vcmp(&hm->uri, "/api/v1/instances") == 0) {
		serve_instances(server, sess, conn, hm);
	}
}






