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

#include <json/json.h>

#include <boost/tokenizer.hpp>
using boost::tokenizer;
using boost::escaped_list_separator;

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

void APIServer::send_json(struct mg_connection *conn, const Json::Value &d) {
	Json::FastWriter writer;
	std::string json = writer.write(d);

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
	Json::Value json;
	json["error"] = error;
	json["message"] = message;

	send_json(conn, json);
}

void APIServer::serve_instances(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	// rapidjson stores const char* pointer to status, so we have to keep
	// the std::string stored out of BOOST_FOREACH scope, otherwise the
	// const char * returned by c_str() would be invalid during send_json.
	std::vector<std::string> statuses;
	std::vector<std::string> usernames;
	std::vector<std::string> list = show_list(m_config, false);

	Json::Value json;
	json["error"] = 0;

	Json::Value instances(Json::arrayValue);
	BOOST_FOREACH(std::string &id, list) {
		Json::Value instance;
		instance["id"] = id;

		std::string name = get_config(m_config, id, "identity.name");
		if (name.empty() || name == "Spectrum 2 Transport") {
			instance["name"] = id;
		}
		else {
			statuses.push_back(name);
			instance["name"] = statuses.back();
		}

		std::string status = server->send_command(id, "status");
		if (status.empty()) {
			status = "Cannot get the instance status.";
		}

		statuses.push_back(status);
		instance["status"] = statuses.back();

		bool running = true;
		if (status.find("Running") == std::string::npos) {
			running = false;
		}
		instance["running"] = running;

		UserInfo info;
		m_storage->getUser(session->user, info);
		std::string username = "";
		int type = (int) TYPE_STRING;
		m_storage->getUserSetting(info.id, id, type, username);

		usernames.push_back(username);
		instance["registered"] = !username.empty();
		instance["username"] = usernames.back();

		usernames.push_back(get_config(m_config, id, "service.frontend"));
		instance["frontend"] = usernames.back();

		instances.append(instance);
	}

	json["instances"] = instances;
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

void APIServer::serve_instances_unregister(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);

	UserInfo info;
	m_storage->getUser(session->user, info);

	std::string username = "";
	int type = (int) TYPE_STRING;
	m_storage->getUserSetting(info.id, instance, type, username);

	if (username.empty() && session->admin) {
		username = get_http_var(hm, "command_arg0");
	}

	if (!username.empty()) {
		std::string response = server->send_command(instance, "unregister " + username);
		if (!response.empty()) {
			username = "";
			m_storage->updateUserSetting(info.id, instance, username);
			send_ack(conn, false, response);
		}
		else {
			send_ack(conn, true, "Unknown error.");
		}
	}
	else {
		send_ack(conn, true, "You are not registered to this Spectrum 2 instance.");
	}
}

void APIServer::serve_instances_register(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);

	UserInfo info;
	m_storage->getUser(session->user, info);

	std::string username = "";
	int type = (int) TYPE_STRING;
	m_storage->getUserSetting(info.id, instance, type, username);

	std::string jid = get_http_var(hm, "jid");
	std::string uin = get_http_var(hm, "uin");
	std::string password = get_http_var(hm, "password");

	// For some networks like IRC, there is no registration.
	// We detect such networks according to registration_fields and use
	// "unknown" uin for them.
	if (uin.empty() || password.empty()) {
		std::string response = server->send_command(instance, "registration_fields");
		std::vector<std::string> fields;
		boost::split(fields, response, boost::is_any_of("\n"));
		if (fields.size() == 1) {
			uin = "unknown";
			password = "unknown";
		}
	}

	if (jid.empty() || uin.empty() || password.empty()) {
		send_ack(conn, true, "Insufficient data.");
	}
	else {
		// Check if the frontend wants to use OAuth2 (Slack for example).
		std::string response = server->send_command(instance, "get_oauth2_url " + jid + " " + uin + " " + password);
		if (!response.empty() && response.find("Error:") != 0) {
			Json::Value json;
			json["error"] = false;
			json["oauth2_url"] = response;
			send_json(conn, json);
		}
		else {
			response = server->send_command(instance, "register " + jid + " " + uin + " " + password);
			if (!response.empty()) {
				std::string value = jid;
				m_storage->updateUserSetting(info.id, instance, value);
				send_ack(conn, false, response);
			}
			else {
				send_ack(conn, true, response);
				return;
			}
		}
	}
}



// void APIServer::serve_instances_register_form(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
// 	std::string uri(hm->uri.p, hm->uri.len);
// 	std::string instance = uri.substr(uri.rfind("/") + 1);
// 
// 	std::string response = server->send_command(instance, "registration_fields");
// 	std::vector<std::string> fields;
// 	boost::split(fields, response, boost::is_any_of("\n"));
// 
// 	if (fields.empty()) {
// 		fields.push_back("Jabber ID");
// 		fields.push_back("3rd-party network username");
// 		fields.push_back("3rd-party network password");
// 	}
// 
// 	Document json;
// 	json.SetObject();
// 	json.AddMember("error", 0, json.GetAllocator());
// 	json.AddMember("username_label", StringRef(fields[0].c_str()), json.GetAllocator());
// 	json.AddMember("legacy_username_label", fields.size() >= 2 ? StringRef(fields[1].c_str()) : "", json.GetAllocator());
// 	json.AddMember("password_label", fields.size() >= 3 ? StringRef(fields[2].c_str()) : "", json.GetAllocator());
// 	send_json(conn, json);
// }

void APIServer::serve_instances_commands(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);

	UserInfo info;
	m_storage->getUser(session->user, info);

	std::string username = "";
	int type = (int) TYPE_STRING;
	m_storage->getUserSetting(info.id, instance, type, username);

	std::string response = server->send_command(instance, "commands");

	std::vector<std::string> commands;
	boost::split(commands, response, boost::is_any_of("\n"));

	Json::Value json;
	json["error"] = 0;

	std::vector<std::vector<std::string> > tmp;
	Json::Value cmds(Json::arrayValue);
	BOOST_FOREACH(const std::string &command, commands) {
		escaped_list_separator<char> els('\\', ' ', '\"');
		tokenizer<escaped_list_separator<char> > tok(command, els);

		std::vector<std::string> tokens;
		for (tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg) {
			tokens.push_back(*beg);
		}

		if (tokens.size() != 11) {
			continue;
		}

		if (!session->admin && tokens[6] == "Admin") {
			continue;
		}

		// Skip command which needs registered users.
		if (!session->admin && username.empty() && tokens[8] == "User") {
			continue;
		}


		// Skip 'register' command when user is registered.
		if (!session->admin && !username.empty() && tokens[0] == "register") {
			continue;
		}

		tmp.push_back(tokens);

		Json::Value cmd;
		cmd["name"] = tokens[0];
		cmd["desc"] = tokens[2];
		cmd["category"] = tokens[4];
		cmd["context"] = tokens[8];
		cmd["label"] = tokens[10];
		cmds.append(cmd);
	}

	json["commands"] = cmds;
	send_json(conn, json);
}

void APIServer::serve_instances_variables(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);

	UserInfo info;
	m_storage->getUser(session->user, info);

	std::string username = "";
	int type = (int) TYPE_STRING;
	m_storage->getUserSetting(info.id, instance, type, username);

	std::string response = server->send_command(instance, "variables");

	std::vector<std::string> commands;
	boost::split(commands, response, boost::is_any_of("\n"));

	Json::Value json;
	json["error"] = 0;

	std::vector<std::vector<std::string> > tmp;
	Json::Value cmds(Json::arrayValue);
	BOOST_FOREACH(const std::string &command, commands) {
		escaped_list_separator<char> els('\\', ' ', '\"');
		tokenizer<escaped_list_separator<char> > tok(command, els);

		std::vector<std::string> tokens;
		for (tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg) {
			tokens.push_back(*beg);
		}

		if (tokens.size() != 13) {
			continue;
		}

		if (!session->admin && tokens[10] == "Admin") {
			continue;
		}

		tmp.push_back(tokens);

		Json::Value cmd;
		cmd["name"] = tokens[0];
		cmd["desc"] = tokens[2];
		cmd["value"] = tokens[4];
		cmd["read_only"] = tokens[6];
		cmd["category"] = tokens[8];
		cmd["context"] = tokens[12];
		cmds.append(cmd);
	}

	json["variables"] = cmds;
	send_json(conn, json);
}


void APIServer::serve_instances_command_args(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	std::string command = get_http_var(hm, "command");
	boost::trim(command);

	std::string response = server->send_command(instance, "commands");

	bool found = false;
	bool userContext = false;
	std::vector<std::string> commands;
	boost::split(commands, response, boost::is_any_of("\n"));
	BOOST_FOREACH(const std::string &cmd, commands) {
		escaped_list_separator<char> els('\\', ' ', '\"');
		tokenizer<escaped_list_separator<char> > tok(cmd, els);

		std::vector<std::string> tokens;
		for (tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg) {
			tokens.push_back(*beg);
		}

		if (tokens.size() != 11) {
			continue;
		}

		std::cout << tokens[0] << " " << command << "\n";
		if (tokens[0] != command) {
			continue;
		}

		if (!session->admin && tokens[6] == "Admin") {
			send_ack(conn, false, "Only admin is able to query this command.");
			return;
		}

		if (tokens[8] == "User") {
			userContext = true;
		}

		found = true;
		break;
	}

	if (!found) {
		command = "unknown";
	}

	response = server->send_command(instance, "args " + command);
	if (response.find("Error:") == 0) {
		send_ack(conn, false, response);
		return;
	}

	

	std::vector<std::string> args;
	boost::split(args, response, boost::is_any_of("\n"));

	Json::Value json;
	json["error"] = 0;

	std::vector<std::vector<std::string> > tmp;
	Json::Value argList(Json::arrayValue);

	if (userContext && session->admin) {
		Json::Value arg;
		arg["name"] = "username";
		arg["label"] = "Username";
		arg["example"] = "";
		argList.append(arg);
	}

	BOOST_FOREACH(const std::string &argument, args) {
		escaped_list_separator<char> els('\\', ' ', '\"');
		tokenizer<escaped_list_separator<char> > tok(argument, els);

		std::vector<std::string> tokens;
		for (tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg) {
			tokens.push_back(*beg);
		}

		if (tokens.size() != 7) {
			continue;
		}

		tmp.push_back(tokens);

		Json::Value arg;
		arg["name"] = tokens[0];
		arg["label"] = tokens[2];
		arg["example"] = tokens[4];
		arg["type"] = tokens[6];
		argList.append(arg);
	}

	json["args"] = argList;
	send_json(conn, json);
}


void APIServer::serve_instances_execute(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	std::string command = get_http_var(hm, "command");
	boost::trim(command);

	std::string response = server->send_command(instance, "commands");

	bool found = false;
	bool userContext = false;
	std::vector<std::string> commands;
	boost::split(commands, response, boost::is_any_of("\n"));
	BOOST_FOREACH(const std::string &cmd, commands) {
		escaped_list_separator<char> els('\\', ' ', '\"');
		tokenizer<escaped_list_separator<char> > tok(cmd, els);

		std::vector<std::string> tokens;
		for (tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg) {
			tokens.push_back(*beg);
		}

		if (tokens.size() != 11) {
			continue;
		}

		std::cout << tokens[0] << " " << command << "\n";
		if (tokens[0] != command) {
			continue;
		}

		if (!session->admin && tokens[6] == "Admin") {
			send_ack(conn, false, "Only admin is able to execute.");
			return;
		}

		if (tokens[8] == "User") {
			userContext = true;
		}

		found = true;
		break;
	}

	if (!found) {
		command = "unknown";
	}

	UserInfo info;
	m_storage->getUser(session->user, info);
	std::string username = "";
	int type = (int) TYPE_STRING;
	m_storage->getUserSetting(info.id, instance, type, username);

	if (userContext && !session->admin) {
		if (username.empty()) {
			send_ack(conn, false, "Error: You are not registered to this transport instance.");
			return;
		}

		command += " " + username;
	}

	for (int i = 0; i < 10; ++i) {
		std::string var = get_http_var(hm, std::string(std::string("command_arg") + boost::lexical_cast<std::string>(i)).c_str());
		if (!var.empty()) {
			command += " " + var;
		}
	}

	response = server->send_command(instance, command);
	if (response.find("Error:") == 0) {
		send_ack(conn, false, response);
	}

	std::vector<std::string> fields;
	boost::split(fields, response, boost::is_any_of("\n"));
	if (!fields.empty() && /*fields[0].find(" - ") != std::string::npos &&*/ (fields[0].find(": ") != std::string::npos || fields[0].find(":\"") != std::string::npos)) {
		Json::Value json;
		json["error"] = 0;

		std::vector<std::string> tmp;
		std::vector<std::string> tmp2;
		Json::Value table(Json::arrayValue);

		BOOST_FOREACH(const std::string &line, fields) {
			escaped_list_separator<char> els('\\', ' ', '\"');
			tokenizer<escaped_list_separator<char> > tok(line, els);

			Json::Value arg;

			std::string key;
			int i = 0;
			bool hasDesc = true;
			for (tokenizer<escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg, ++i) {
				if (i == 1 && *beg != "-") {
					hasDesc = false;
				}
				if (i == 0) {
					tmp.push_back(*beg);
					arg["Key"] = tmp.back();
				}
				else if (i == 2 && hasDesc) {
					tmp.push_back(*beg);
					arg["Description"] = tmp.back();
				}
				else if (i > 1 || (!hasDesc && i > 0)) {
					if (key.empty()) {
						key = *beg;
					}
					else {
						tmp.push_back(key);
						tmp2.push_back(*beg);
						arg[tmp.back()] = tmp2.back();
						key = "";
					}
				}
			}
			table.append(arg);
		}

		json["table"] = table;
		json["message"] = response;
		send_json(conn, json);
	}
	else {
		boost::replace_all(response, "\n", "<br/>");
		send_ack(conn, true, response);
	}
}

void APIServer::serve_users(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	ALLOW_ONLY_ADMIN();

	Json::Value json;
	json["error"] = 0;

	std::vector<std::string> list;
	m_storage->getUsers(list);

	Json::Value users(Json::arrayValue);
	BOOST_FOREACH(std::string &id, list) {
		Json::Value user;
		user["username"] = id;
		users.append(user);
	}

	json["users"] = users;
	send_json(conn, json);
}

void APIServer::serve_users_add(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	std::string user = get_http_var(hm, "username");
	std::string password = get_http_var(hm, "password");

	if (!user.empty() && !password.empty()) {
		if (m_storage) {
			UserInfo dummy;
			bool registered = m_storage->getUser(user, dummy);
			if (!registered) {
				UserInfo info;
				info.jid = user;
				info.password = password;
				m_storage->setUser(info);
			}
			else {
				send_ack(conn, true, "This user is already registered");
				return;
			}
		}
		else {
			send_ack(conn, false, "Storage backend is not configured.");
		}
	}
	else {
		send_ack(conn, true, "Username or password has not been provided.");
		return;
	}
	send_ack(conn, false, "");
}

void APIServer::serve_users_remove(Server *server, Server::session *session, struct mg_connection *conn, struct http_message *hm) {
	ALLOW_ONLY_ADMIN();

	std::string uri(hm->uri.p, hm->uri.len);
	std::string user = uri.substr(uri.rfind("/") + 1);

	if (!m_storage) {
		return;
	}

	UserInfo info;
	m_storage->getUser(user, info);
	m_storage->removeUser(info.id);

	send_ack(conn, false, "");
}

void APIServer::handleRequest(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm) {
	if (has_prefix(&hm->uri, "/api/v1/instances/start/")) {
		serve_instances_start(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/stop/")) {
		serve_instances_stop(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/unregister/")) {
		serve_instances_unregister(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/register/")) {
		serve_instances_register(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/commands/")) {
		serve_instances_commands(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/variables/")) {
		serve_instances_variables(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/command_args/")) {
		serve_instances_command_args(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/instances/execute/")) {
		serve_instances_execute(server, sess, conn, hm);
	}
	else if (has_prefix(&hm->uri, "/api/v1/users/remove/")) {
		serve_users_remove(server, sess, conn, hm);
	}
	else if (mg_vcmp(&hm->uri, "/api/v1/users/add") == 0) {
		serve_users_add(server, sess, conn, hm);
	}
	else if (mg_vcmp(&hm->uri, "/api/v1/users") == 0) {
		serve_users(server, sess, conn, hm);
	}
	else if (mg_vcmp(&hm->uri, "/api/v1/instances") == 0) {
		serve_instances(server, sess, conn, hm);
	}
}






