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

#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#define ALLOW_ONLY_ADMIN() 	if (!session->admin) { \
		send_ack(conn, true, "Only administrators can do this API call."); \
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

void APIServer::send_json(struct mg_connection *conn, const Document &d) {
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	d.Accept(writer);
	std::string json(buffer.GetString());

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

void APIServer::send_ack(struct mg_connection *conn, bool error, const std::string &message) {
	Document json;
	json.SetObject();
	json.AddMember("error", error, json.GetAllocator());
	json.AddMember("message", message.c_str(), json.GetAllocator());

	send_json(conn, json);
}

void APIServer::serve_instances(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
// 	std::string jid = get_http_var(hm, "jid");
// 	if (!jid.empty()) {
// 		serve_instance(conn, hm, jid);
// 		return;
// 	}

	std::vector<std::string> list = show_list(m_config, false);

	Document json;
	json.SetObject();
	json.AddMember("error", 0, json.GetAllocator());

	Value instances(kArrayType);
	BOOST_FOREACH(std::string &id, list) {
		Value instance;
		instance.SetObject();
		instance.AddMember("id", id.c_str(), json.GetAllocator());
		instance.AddMember("name", id.c_str(), json.GetAllocator());

		std::string status = server->send_command(id, "status");
		if (status.empty()) {
			status = "Cannot get the instance status.";
		}
		else if (*(status.end() - 1) == '\n') {
			status.erase(status.end() - 1);
		}
		instance.AddMember("status", status.c_str(), json.GetAllocator());

		bool running = true;
		if (status.find("Running") == std::string::npos) {
			running = false;
		}
		instance.AddMember("running", running, json.GetAllocator());

		UserInfo info;
		m_storage->getUser(session->user, info);
		std::string username = "";
		int type = (int) TYPE_STRING;
		m_storage->getUserSetting(info.id, id, type, username);

		instance.AddMember("registered", !username.empty(), json.GetAllocator());
		instance.AddMember("username", username.c_str(), json.GetAllocator());

		instances.PushBack(instance, json.GetAllocator());
	}

	json.AddMember("instances", instances, json.GetAllocator());
	send_json(conn, json);
}

void APIServer::serve_instances_start(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	ALLOW_ONLY_ADMIN();

	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	start_instances(m_config, instance);
	std::string response = get_response();

	// TODO: So far it needs some time to reload Spectrum 2, so just sleep here.
	sleep(1);

	send_ack(conn, response.find("OK") == std::string::npos, response);
}

void APIServer::serve_instances_stop(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	ALLOW_ONLY_ADMIN();

	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	stop_instances(m_config, instance);
	std::string response = get_response();
	send_ack(conn, response.find("OK") == std::string::npos, response);
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






