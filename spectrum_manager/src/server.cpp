#include "APIServer.h"
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

DEFINE_LOGGER(logger, "Server");

static struct mg_serve_http_opts s_http_server_opts;

namespace ServerUtils {

static int has_prefix(const struct mg_str *uri, const char *prefix) {
	size_t prefix_len = strlen(prefix);
	return uri->len >= prefix_len && memcmp(uri->p, prefix, prefix_len) == 0;
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

	struct mg_bind_opts opts;
	memset(&opts, 0, sizeof(opts));
	const char *error_string;
	opts.error_string = &error_string;
	m_nc = mg_bind_opt(&m_mgr, std::string(":" + boost::lexical_cast<std::string>(CONFIG_INT(m_config, "service.port"))).c_str(), &_event_handler, opts);
	if (!m_nc) {
		std::cerr << "Error creating server: " << error_string << "\n";
		exit(1);
	}
#ifdef MG_ENABLE_SSL
	if (!CONFIG_STRING(m_config, "service.cert").empty()) {
		const char *err_str = mg_set_ssl(m_nc, CONFIG_STRING(m_config, "service.cert").c_str(), NULL);
		if (err_str) {
			std::cerr << "Error setting SSL certificate: " << err_str << "\n";
			exit(1);
		}
	}
#endif
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

	std::ofstream output;
	output.open(std::string(CONFIG_STRING(config, "service.data_dir") + "/js/config.js").c_str(), std::ios::out);
	if (output.fail()) {
		LOG4CXX_ERROR(logger, "Cannot open " << std::string(CONFIG_STRING(config, "service.data_dir") + "/js/config.js") << " for writing: " << strerror(errno));
	}
	output << "var BaseLocation = \"" << CONFIG_STRING(m_config, "service.base_location") << "\";\n";
	output.close();

	m_storageCfg = new Config();
	m_storageCfg->load(config_file);
	
	Logging::initManagerLogging(m_storageCfg);
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

	m_apiServer = new APIServer(config, m_storage);
}

Server::~Server() {
	delete m_apiServer;

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
	std::string user = ServerUtils::get_http_var(hm, "user");
	std::string password = ServerUtils::get_http_var(hm, "password");

	std::string host;
	mg_str *host_hdr = mg_get_http_header(hm, "Host");
	if (host_hdr) {
		if (!CONFIG_STRING(m_config, "service.cert").empty()) {
			host += "https://";
		}
		else {
			host += "http://";
		}
		host += std::string(host_hdr->p, host_hdr->len);
	}

	if (check_password(user, password) && (session = new_session(user)) != NULL) {
		std::cout << "User authorized\n";
		mg_printf(conn, "HTTP/1.1 302 Found\r\n"
			"Set-Cookie: session=%s; max-age=3600; http-only\r\n"  // Session ID
			"Set-Cookie: user=%s\r\n"  // Set user, needed by Javascript code
			"Set-Cookie: admin=%s\r\n"  // Set user, needed by Javascript code
			"Set-Cookie: base_location=%s\r\n"  // Set user, needed by Javascript code
			"Set-Cookie: original_url=/; max-age=0\r\n"  // Delete original_url
			"Location: %s%sinstances\r\n\r\n",
			session->session_id, session->user, session->admin ? "1" : "0", CONFIG_STRING(m_config, "service.base_location").c_str(), host.c_str(), CONFIG_STRING(m_config, "service.base_location").c_str());
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
		!mg_vcmp(&hm->uri, "/js/jquery.js") ||
		!mg_vcmp(&hm->uri, "/js/jquery-ui.js") ||
		!mg_vcmp(&hm->uri, "/js/jquery.cookie.js") ||
		!mg_vcmp(&hm->uri, "/js/config.js") ||
		!mg_vcmp(&hm->uri, "/js/app.js") ||
		!mg_vcmp(&hm->uri, "/users/register.shtml") ||
		!mg_vcmp(&hm->uri, "/api/v1/users/add") ||
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
	std::string host;
	mg_str *host_hdr = mg_get_http_header(hm, "Host");
	if (host_hdr) {
		if (!CONFIG_STRING(m_config, "service.cert").empty()) {
			host += "https://";
		}
		else {
			host += "http://";
		}
		host += std::string(host_hdr->p, host_hdr->len);
	}

	where = where + 1;

	mg_printf(conn, "HTTP/1.1 302 Found\r\n"
		"Set-Cookie: original_url=/\r\n"
		"Location: %s%s%s\r\n\r\n", host.c_str(), CONFIG_STRING(m_config, "service.base_location").c_str(), where);
}

void Server::print_html(struct mg_connection *conn, struct http_message *hm, const std::string &html) {
	mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %zu\r\n"        // Always set Content-Length
			"\r\n"
			"%s%s%s",
			(int) html.size() + m_header.size() + m_footer.size(), m_header.c_str(), html.c_str(), m_footer.c_str());
}

std::string Server::send_command(const std::string &jid, const std::string &cmd, int timeout) {
	Swift::SimpleEventLoop eventLoop;
	Swift::BoostNetworkFactories networkFactories(&eventLoop);

	try {
		ask_local_server(m_config, networkFactories, jid, cmd);
		struct timeval td_start, td_end;

		gettimeofday(&td_start, NULL);
		gettimeofday(&td_end, NULL);

		while (get_response().empty() && td_end.tv_sec - td_start.tv_sec < timeout) {
			gettimeofday(&td_end, NULL);
			eventLoop.runOnce();
		}
	}
	catch (const boost::bad_any_cast &e) {
		LOG4CXX_ERROR(logger, "Server::send_command: bad_any_cast: " << e.what());
		return "";
	}

	std::string response = get_response();
	if (response == "Empty response") {
		return "";
	}

	return response;
}

// void Server::serve_onlineusers(struct mg_connection *conn, struct http_message *hm) {
// 	Server:session *session = get_session(hm);
// 	if (!session->admin) {
// 		redirect_to(conn, hm, "/");
// 		return;
// 	}
// 
// 	std::string html;
// 	std::string jid = get_http_var(hm, "jid");
// 
// 	html += std::string("<h2>") + jid + " online users</h2><table><tr><th>JID<th>Command</th></tr>";
// 
// 	Swift::SimpleEventLoop eventLoop;
// 	Swift::BoostNetworkFactories networkFactories(&eventLoop);
// 
// 	ask_local_server(m_config, networkFactories, jid, "online_users");
// 	eventLoop.runUntilEvents();
// 	while (get_response().empty()) {
// 		eventLoop.runUntilEvents();
// 	}
// 
// 	std::string response = get_response();
// 	std::vector<std::string> users;
// 	boost::split(users, response, boost::is_any_of("\n"));
// 
// 	BOOST_FOREACH(std::string &user, users) {
// 		html += "<tr><td>" + user + "</td><td></td></tr>";
// 	}
// 
// 	html += "</table><a href=\"/\">Back to main page</a>";
// 	html += "</body></html>";
// 	print_html(conn, hm, html);
// }
// 
// void Server::serve_cmd(struct mg_connection *conn, struct http_message *hm) {
// 	Server:session *session = get_session(hm);
// 	if (!session->admin) {
// 		redirect_to(conn, hm, "/");
// 		return;
// 	}
// 
// 	std::string html;
// 	std::string jid = get_http_var(hm, "jid");
// 	std::string cmd = get_http_var(hm, "cmd");
// 
// 	html += std::string("<h2>") + jid + " command result</h2>";
// 
// 	Swift::SimpleEventLoop eventLoop;
// 	Swift::BoostNetworkFactories networkFactories(&eventLoop);
// 
// 	ask_local_server(m_config, networkFactories, jid, cmd);
// 	while (get_response().empty()) {
// 		eventLoop.runUntilEvents();
// 	}
// 
// 	std::string response = get_response();
// 	
// 	html += "<pre>" + response + "</pre>";
// 
// 	html += "<a href=\"/\">Back to main page</a>";
// 	html += "</body></html>";
// 	print_html(conn, hm, html);
// }

void Server::serve_logout(struct mg_connection *conn, struct http_message *hm) {
	std::string host;
	mg_str *host_hdr = mg_get_http_header(hm, "Host");
	if (host_hdr) {
		if (!CONFIG_STRING(m_config, "service.cert").empty()) {
			host += "https://";
		}
		else {
			host += "http://";
		}
		host += std::string(host_hdr->p, host_hdr->len);
	}

	Server::session *session = get_session(hm);
	mg_printf(conn, "HTTP/1.1 302 Found\r\n"
		"Set-Cookie: session=%s; max-age=0\r\n"
		"Set-Cookie: admin=%s; max-age=0\r\n"
		"Location: %s%s\r\n\r\n",
		session->session_id, session->admin ? "1" : "0", host.c_str(), CONFIG_STRING(m_config, "service.base_location").c_str());

	sessions.erase(session->session_id);
	delete session;
}

void Server::serve_oauth2(struct mg_connection *conn, struct http_message *hm) {
// 	http://slack.spectrum.im/oauth2/localhostxmpp?code=14830663267.19140123492.e7f78a836d&state=534ab3b6-8bf1-4974-8274-847df8490bc5
	std::string uri(hm->uri.p, hm->uri.len);
	std::string instance = uri.substr(uri.rfind("/") + 1);
	std::string code = ServerUtils::get_http_var(hm, "code");
	std::string state = ServerUtils::get_http_var(hm, "state");

	std::string response = send_command(instance, "set_oauth2_code " + code + " " + state, 30);
	std::cerr << "set_oauth2_code response: '" << response << "'\n";
	if (response.find("Registered as ") == 0) {
		std::vector<std::string> args;
		boost::split(args, response, boost::is_any_of(" "));
		std::cerr << "set_oauth2_code response size " << args.size() << "\n";
		if (args.size() == 3) {
			Server::session *session = get_session(hm);
			UserInfo info;
			m_storage->getUser(session->user, info);
			std::string username = "";
			int type = (int) TYPE_STRING;
			m_storage->getUserSetting(info.id, instance, type, username);
			m_storage->updateUserSetting(info.id, instance, args[2]);
		}
	}
	redirect_to(conn, hm, "/instances/");
}

void Server::event_handler(struct mg_connection *conn, int ev, void *p) {
	struct http_message *hm = (struct http_message *) p;

	if (ev == MG_EV_SSI_CALL) {
		mbuf_resize(&conn->send_mbuf, conn->send_mbuf.size * 2);
		std::string resp(conn->send_mbuf.buf, conn->send_mbuf.len);
		boost::replace_all(resp, "href=\"/", std::string("href=\"") + CONFIG_STRING(m_config, "service.base_location"));
		boost::replace_all(resp, "src=\"/", std::string("src=\"") + CONFIG_STRING(m_config, "service.base_location"));
		boost::replace_all(resp, "action=\"/", std::string("action=\"") + CONFIG_STRING(m_config, "service.base_location"));
		strcpy(conn->send_mbuf.buf, resp.c_str());
		conn->send_mbuf.len = resp.size();
		mbuf_trim(&conn->send_mbuf);
		return;
	}

	if (ev != MG_EV_HTTP_REQUEST) {
		return;
	}

	hm->uri.p += CONFIG_STRING(m_config, "service.base_location").size() - 1;
	hm->uri.len -= CONFIG_STRING(m_config, "service.base_location").size() - 1;

	if (!is_authorized(conn, hm)) {
		redirect_to(conn, hm, "/login");
	} else if (mg_vcmp(&hm->uri, "/authorize") == 0) {
		authorize(conn, hm);
	} else if (mg_vcmp(&hm->uri, "/logout") == 0) {
		serve_logout(conn, hm);
// 	} else if (mg_vcmp(&hm->uri, "/users") == 0) {
// 		serve_users(conn, hm);
// 	} else if (mg_vcmp(&hm->uri, "/users/add") == 0) {
// 		serve_users_add(conn, hm);
// 	} else if (mg_vcmp(&hm->uri, "/users/remove") == 0) {
// 		serve_users_remove(conn, hm);
	} else if (ServerUtils::has_prefix(&hm->uri, "/oauth2")) {
		serve_oauth2(conn, hm);
	} else if (ServerUtils::has_prefix(&hm->uri, "/api/v1/")) {
		m_apiServer->handleRequest(this, get_session(hm), conn, hm);
	} else {
		if (hm->uri.p[hm->uri.len - 1] != '/') {
			std::string url(hm->uri.p, hm->uri.len);
			if (url.find(".") == std::string::npos) {
				url += "/";
				redirect_to(conn, hm, url.c_str());
				conn->flags |= MG_F_SEND_AND_CLOSE;
				return;
			}
		}
		mg_serve_http(conn, hm, s_http_server_opts);
	}

	conn->flags |= MG_F_SEND_AND_CLOSE;
}




