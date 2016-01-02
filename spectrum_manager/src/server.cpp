#include "server.h"
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

#define SESSION_TTL 120

static struct mg_serve_http_opts s_http_server_opts;


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

static void my_strlcpy(char *dst, const char *src, size_t len) {
  strncpy(dst, src, len);
  dst[len - 1] = '\0';
}

// Generate session ID. buf must be 33 bytes in size.
// Note that it is easy to steal session cookies by sniffing traffic.
// This is why all communication must be SSL-ed.
static void generate_session_id(char *buf, const char *random,
                                const char *user) {
  cs_md5(buf, random, strlen(random), user, strlen(user), NULL);
}

static void _event_handler(struct mg_connection *nc, int ev, void *p) {
	static_cast<Server *>(nc->mgr->user_data)->event_handler(nc, ev, p);
}

Server::Server(ManagerConfig *config, const std::string &config_file) {
	srand((unsigned) time(0));
	m_config = config;
	m_user = CONFIG_STRING(m_config, "service.admin_username");
	m_password = CONFIG_STRING(m_config, "service.admin_password");

	mg_mgr_init(&m_mgr, this);
	m_nc = mg_bind(&m_mgr, std::string(":" + boost::lexical_cast<std::string>(CONFIG_INT(m_config, "service.port"))).c_str(), &_event_handler);
	mg_set_protocol_http_websocket(m_nc);

	s_http_server_opts.document_root = CONFIG_STRING(m_config, "service.data_dir").c_str();

	std::ifstream header(std::string(CONFIG_STRING(m_config, "service.data_dir") + "/header.html").c_str(), std::ios::in);
	if (header) {
		header.seekg(0, std::ios::end);
		m_header.resize(header.tellg());
		header.seekg(0, std::ios::beg);
		header.read(&m_header[0], m_header.size());
		header.close();
	}

	std::ifstream footer(std::string(CONFIG_STRING(m_config, "service.data_dir") + "/footer.html").c_str(), std::ios::in);
	if (footer) {
		footer.seekg(0, std::ios::end);
		m_footer.resize(footer.tellg());
		footer.seekg(0, std::ios::beg);
		footer.read(&m_footer[0], m_footer.size());
		footer.close();
	}

	m_storageCfg = new Config();
	m_storageCfg->load(config_file);

	std::string error;
	m_storage = StorageBackend::createBackend(m_storageCfg, error);
	if (m_storage == NULL) {
		std::cerr << "Error creating StorageBackend! " << error << "\n";
		std::cerr << "Registering new Spectrum 2 manager users won't work" << "\n";
	}
	else if (!m_storage->connect()) {
		delete m_storage;
		m_storage = NULL;
		std::cerr << "Can't connect to database!\n";
	}
}

Server::~Server() {
	mg_mgr_free(&m_mgr);
	if (m_storage) {
		delete m_storage;
	}
	delete m_storageCfg;
}

bool Server::start() {
	for (;;) {
		mg_mgr_poll(&m_mgr, 1000);
	}

	return true;
}

bool Server::check_password(const std::string &user, const std::string &password) {
	if (m_user == user && m_password == password) {
		return true;
	}

	UserInfo info;
	if (m_storage && m_storage->getUser(user, info) == true && info.password == password) {
		return true;
	}

	return false;
}

// Allocate new session object
Server::session *Server::new_session(const std::string &user) {
	Server::session *session = new Server::session;

	my_strlcpy(session->user, user.c_str(), sizeof(session->user));
	snprintf(session->random, sizeof(session->random), "%d", rand());
	generate_session_id(session->session_id, session->random, session->user);
	session->expire = time(0) + SESSION_TTL;
	session->admin = std::string(user) == m_user;

	sessions[session->session_id] = session;
	return session;
}

// Get session object for the connection. Caller must hold the lock.
Server::session *Server::get_session(struct http_message *hm) {
	time_t now = time(NULL);
	char session_id[255];
	struct mg_str *hdr = mg_get_http_header(hm, "Cookie");
	int len = mg_http_parse_header(hdr, "session", session_id, sizeof(session_id));
	session_id[len] = 0;

	if (sessions.find(session_id) == sessions.end()) {
		return NULL;
	}

	if (sessions[session_id]->expire != 0 && sessions[session_id]->expire > now) {
		return sessions[session_id];
	}

	return NULL;
}

void Server::authorize(struct mg_connection *conn, struct http_message *hm) {
	Server::session *session;
	std::string user = get_http_var(hm, "user");
	std::string password = get_http_var(hm, "password");

	if (check_password(user, password) && (session = new_session(user)) != NULL) {
		std::cout << "User authorized\n";
		mg_printf(conn, "HTTP/1.1 302 Found\r\n"
			"Set-Cookie: session=%s; max-age=3600; http-only\r\n"  // Session ID
			"Set-Cookie: user=%s\r\n"  // Set user, needed by Javascript code
			"Set-Cookie: original_url=/; max-age=0\r\n"  // Delete original_url
			"Location: /\r\n\r\n",
			session->session_id, session->user);
	} else {
		// Authentication failure, redirect to login.
		redirect_to(conn, hm, "/login");
	}
}

bool Server::is_authorized(const struct mg_connection *conn, struct http_message *hm) {
	Server::session *session;
	char valid_id[33];
	bool authorized = false;

	// Always authorize accesses to login page and to authorize URI
	if (!mg_vcmp(&hm->uri, "/login") ||
		!mg_vcmp(&hm->uri, "/login/") ||
		!mg_vcmp(&hm->uri, "/form.css") ||
		!mg_vcmp(&hm->uri, "/style.css") ||
		!mg_vcmp(&hm->uri, "/logo.png") ||
		!mg_vcmp(&hm->uri, "/authorize")) {
		return true;
	}

	if ((session = get_session(hm)) != NULL) {
		generate_session_id(valid_id, session->random, session->user);
		if (strcmp(valid_id, session->session_id) == 0) {
			session->expire = time(0) + SESSION_TTL;
			authorized = true;
		}
	}

	return authorized;
}

void Server::redirect_to(struct mg_connection *conn, struct http_message *hm, const char *where) {
	mg_printf(conn, "HTTP/1.1 302 Found\r\n"
		"Set-Cookie: original_url=/\r\n"
		"Location: %s\r\n\r\n", where);
}

void Server::print_html(struct mg_connection *conn, struct http_message *hm, const std::string &html) {
	mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %d\r\n"        // Always set Content-Length
			"\r\n"
			"%s%s%s",
			(int) html.size() + m_header.size() + m_footer.size(), m_header.c_str(), html.c_str(), m_footer.c_str());
}

std::string Server::send_command(const std::string &jid, const std::string &cmd) {
	Swift::SimpleEventLoop eventLoop;
	Swift::BoostNetworkFactories networkFactories(&eventLoop);

	ask_local_server(m_config, networkFactories, jid, cmd);
	eventLoop.runUntilEvents();
	struct timeval td_start,td_end;
	float elapsed = 0; 
	gettimeofday(&td_start, NULL);

	time_t started = time(NULL);
	while(get_response().empty() && td_end.tv_sec - td_start.tv_sec < 1) {
		gettimeofday(&td_end, NULL);
		eventLoop.runOnce();
	}

	return get_response();
}

void Server::serve_onlineusers(struct mg_connection *conn, struct http_message *hm) {
	std::string html;
	std::string jid = get_http_var(hm, "jid");

	html += std::string("<h2>") + jid + " online users</h2><table><tr><th>JID<th>Command</th></tr>";

	Swift::SimpleEventLoop eventLoop;
	Swift::BoostNetworkFactories networkFactories(&eventLoop);

	ask_local_server(m_config, networkFactories, jid, "online_users");
	eventLoop.runUntilEvents();
	while(get_response().empty()) {
		eventLoop.runUntilEvents();
	}

	std::string response = get_response();
	std::vector<std::string> users;
	boost::split(users, response, boost::is_any_of("\n"));

	BOOST_FOREACH(std::string &user, users) {
		html += "<tr><td>" + user + "</td><td></td></tr>";
	}

	html += "</table><a href=\"/\">Back to main page</a>";
	html += "</body></html>";
	print_html(conn, hm, html);
}

void Server::serve_cmd(struct mg_connection *conn, struct http_message *hm) {
	std::string html;
	std::string jid = get_http_var(hm, "jid");
	std::string cmd = get_http_var(hm, "cmd");

	html += std::string("<h2>") + jid + " command result</h2>";

	Swift::SimpleEventLoop eventLoop;
	Swift::BoostNetworkFactories networkFactories(&eventLoop);

	ask_local_server(m_config, networkFactories, jid, cmd);
	while(get_response().empty()) {
		eventLoop.runUntilEvents();
	}

	std::string response = get_response();
	
	html += "<pre>" + response + "</pre>";

	html += "<a href=\"/\">Back to main page</a>";
	html += "</body></html>";
	print_html(conn, hm, html);
}

void Server::serve_users_add(struct mg_connection *conn, struct http_message *hm) {
	std::string user = get_http_var(hm, "user");
	std::string password = get_http_var(hm, "password");

	if (!user.empty() && !password.empty()) {
		UserInfo info;
		info.jid = user;
		info.password = password;
		if (m_storage) {
			m_storage->setUser(info);
		}
	}
	redirect_to(conn, hm, "/users");
}

void Server::serve_users(struct mg_connection *conn, struct http_message *hm) {
	std::string html = "<h2>Spectrum 2 manager users</h2>";

	html += "<p>Here, you can add new users who will have access to this web interface. "
			"These users will be able to register new accounts on all Spectrum 2 instances "
			"running on these server. They won't be able to change any Spectrum 2 instance "
			"configuration influencing other users.</p>";

	if (!m_storage) {
		print_html(conn, hm, html);
		return;
	}

	html += "<form action=\"/users/add\" class=\"basic-grey\" method=\"POST\"> \
	<h1>Add user \
		<span>Add new user to Spectrum 2 manager web interface.</span> \
	</h1> \
	<label> \
		<span>Username:</span> \
		<input type=\"text\" id=\"user\" name=\"user\"placeholder=\"Username\"></textarea> \
	</label> \
	<label><span>Password:</span> \
		<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Password\"></textarea> \
	</label> \
	<label> \
		<span>&nbsp;</span> \
		<input type=\"submit\" class=\"button\" value=\"Add user\" />\
	</label> \
</form><br/>";
	std::vector<std::string> users;
	m_storage->getUsers(users);

	html += "<table><tr><th>User<th>Action</th></tr>";
	BOOST_FOREACH(std::string &jid, users) {
		html += "<tr>";
		html += "<td><a href=\"/users?jid=" + jid + "\">" + jid + "</a></td>";
		html += "<td> </td>";
		html += "</tr>";
	}
	html += "</table>";

	print_html(conn, hm, html);
}

void Server::serve_instances_start(struct mg_connection *conn, struct http_message *hm) {
	std::string html;
	std::string jid = get_http_var(hm, "jid");
	if (jid.empty()) {
		redirect_to(conn, hm, "/");
		return;
	}

	start_instances(m_config, jid);
	html += "<h2>Starting Spectrum 2 instance</h2>";
	html += "<b>" + get_response() + "</b><br/><a href=\"/\">Back to main page</a>";
	print_html(conn, hm, html);
}

void Server::serve_instances_stop(struct mg_connection *conn, struct http_message *hm) {
	std::string html;
	std::string jid = get_http_var(hm, "jid");

	stop_instances(m_config, jid);
	html += "<b>" + get_response() + "</b><br/><a href=\"/\">Back to main page</a>";
	html += "</body></html>";
	print_html(conn, hm, html);
}

void Server::serve_instance(struct mg_connection *conn, struct http_message *hm, const std::string &jid) {
	std::string html = "<h2>Spectrum 2 instance: " + jid + "</h2>";

	print_html(conn, hm, html);
}

void Server::serve_instances_unregister(struct mg_connection *conn, struct http_message *hm) {
	
}

void Server::serve_instances_register(struct mg_connection *conn, struct http_message *hm) {
	std::string instance = get_http_var(hm, "instance");
	if (!instance.empty()) {
		serve_instance(conn, hm, instance);
		return;
	}

	std::string jid = get_http_var(hm, "jid");
	std::string uin = get_http_var(hm, "uin");
	std::string password = get_http_var(hm, "password");
	Server:session *session = get_session(hm);
	UserInfo info;
	m_storage->getUser(session->user, info);

	if (uin.empty() || password.empty()) {
		std::string html = "<h2>Register Spectrum 2 instance</h2>";
		html += "<form action=\"/instances/register\" class=\"basic-grey\" method=\"POST\"> \
			<h1>Register Spectrum 2 instance \
				<span>Write the Slack team name, 3rd-party network username and password.</span> \
			</h1> \
			<label> \
				<span>Slack team name:</span> \
				<input type=\"text\" id=\"jid\" name=\"jid\"placeholder=\"Slack team name\"></textarea> \
			</label> \
			<label> \
				<span>3rd-party network username:</span> \
				<input type=\"text\" id=\"uin\" name=\"uin\"placeholder=\"3rd-party network username\"></textarea> \
			</label> \
			<label><span>Password:</span> \
				<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"3rd-party network password\"></textarea> \
			</label> \
			<label> \
				<span>&nbsp;</span> \
				<input type=\"submit\" class=\"button\" value=\"Register\" />\
			</label> \
			</form><br/>";
		print_html(conn, hm, html);
	}
	else {
		std::string response = send_command(jid, "register " + jid + " " + uin + " " + password);
		if (!response.empty()) {
			std::string value = jid;
			int type = (int) TYPE_STRING;
			m_storage->updateUserSetting(info.id, jid, value);
		}
		redirect_to(conn, hm, "/instances");
	}

}

void Server::serve_instances(struct mg_connection *conn, struct http_message *hm) {
	std::string jid = get_http_var(hm, "jid");
	if (!jid.empty()) {
		serve_instance(conn, hm, jid);
		return;
	}

	std::vector<std::string> list = show_list(m_config, false);
	std::string html = "<h2>List of Spectrum 2 instances</h2>";

	Server:session *session = get_session(hm);

	if (session->admin) {
		if (list.empty()) {
			html += "<p>There are no Spectrum 2 instances yet. You can create new instance by adding configuration files into <pre>/etc/spectrum2/transports</pre> directory. You can then maintain the Spectrum 2 instance here.</p>";
		}
		else {
			html += "<table><tr><th>Hostname<th>Status</th><th>Command</th><th>Run command</th></tr>";
			BOOST_FOREACH(std::string &instance, list) {
				html += "<tr>";
				html += "<td><a href=\"/instances?jid=" + instance + "\">" + instance + "</a></td>";

				std::string response = send_command(instance, "status");
				if (response.empty()) {
					response = "Cannot get the server status";
				}

				html += "<td>" + response + "</td>";
				if (response.find("Running") == 0) {
					html += "<td><a href=\"/instances/stop?jid=" + instance + "\">Stop</a></td>";
					html += "<td><form action=\"/cmd\">";
					html += "<input type=\"hidden\" name=\"jid\" value=\"" + instance + "\"></input>";
					html += "<input type=\"text\" name=\"cmd\"></input>";
					html += "<input type=\"submit\" value=\"Run\"></input>";
					html += "</form></td>";
				}
				else {
					html += "<td><a href=\"/instances/start?jid=" + instance + "\">Start</a></td>";
					html += "<td></td>";
				}

				html += "</tr>";
			}

			html += "</table>";
		}
	}
	else {
		UserInfo info;
		m_storage->getUser(session->user, info);

		if (list.empty()) {
			html += "<p>There are no Spectrum 2 instances yet.</p>";
		}
		else {
			html += "<table><tr><th>Hostname<th>Status</th><th>Action</th></tr>";
			BOOST_FOREACH(std::string &instance, list) {
				html += "<tr>";
				html += "<td><a href=\"/instances?jid=" + instance + "\">" + instance + "</a></td>";

				std::string response = send_command(instance, "status");
				if (response.empty()) {
					response = "Cannot get the server status";
				}

				html += "<td>" + response + "</td>";
				if (response.find("Running") == 0) {
					std::string value = "";
					int type = (int) TYPE_STRING;
					m_storage->getUserSetting(info.id, instance, type, value);

					if (!value.empty()) {
						html += "<td><a href=\"/instances/unregister?instance=" + instance + "\">Unregister</a></td>";
					}
					else {
						html += "<td><a href=\"/instances/register?instance=" + instance + "\">Register</a></td>";
					}
				}
				else {
					html += "<td>No available action</td>";
				}

				html += "</tr>";
			}

			html += "</table>";
		}
	}
	print_html(conn, hm, html);
}

void Server::event_handler(struct mg_connection *conn, int ev, void *p) {
	struct http_message *hm = (struct http_message *) p;

	if (ev != MG_EV_HTTP_REQUEST) {
		return;
	}

	if (!is_authorized(conn, hm)) {
		redirect_to(conn, hm, "/login");
	} else if (mg_vcmp(&hm->uri, "/authorize") == 0) {
		authorize(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/") == 0) {
		serve_instances(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/instances") == 0) {
		serve_instances(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/onlineusers") == 0) {
		serve_onlineusers(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/cmd") == 0) {
		serve_cmd(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/instances/start") == 0) {
		serve_instances_start(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/instances/stop") == 0) {
		serve_instances_stop(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/instances/register") == 0) {
		serve_instances_register(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/instances/unregister") == 0) {
		serve_instances_unregister(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/users") == 0) {
		serve_users(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/users/add") == 0) {
		serve_users_add(conn, hm);
	} else {
		mg_serve_http(conn, hm, s_http_server_opts);
	}

	conn->flags |= MG_F_SEND_AND_CLOSE;
}




