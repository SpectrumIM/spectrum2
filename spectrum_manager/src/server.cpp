#include "server.h"
#include "methods.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

#define SESSION_TTL 120

static std::string get_header() {
return "\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\
  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\"> \
<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" dir=\"ltr\"> \
  <head>\
    <title>Spectrum 2 web interface</title>\
    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"/>\
  <style type=\"text/css\">\
body{ background-color: #F9F9F9; color: #444444; font: normal normal 14px \"Helvetica\", \"Arial\", Sans-Serif; }\
\
pre, kbd, var, samp, tt{ font-family: \"Courier\", Monospace; }\
\
pre { font-size: 12px; }\
\
h1, h2, h3, h4, h5, h6, pre{ color: #094776; }\
\
h1{ font-size: 28px; }\
\
h2{ font-size: 24px; font-weight: normal; }\
\
h1, h2, h3, h4, h5, h6{ margin-bottom: 20px; }\
\
h2, h3{ border-bottom: 2px solid #EEEEEE; padding: 0 0 3px; }	\
\
h3{ border-color: #E5E5E5; border-width: 1px; }\
\
h4{ font-size: 18px; }\
\
	h1 a, h2 a{ font-weight: normal; }\
\
	h1 a, h2 a, h3 a{ text-decoration: none; }\
\
h3, h5, h6{ font-size: 18px; }\
\
	h4, h5, h6{ font-size: 14px; }\
\
p, dl, ul, ol{ margin: 20px 0; }\
\
	p, dl, ul, ol, h3, h4, h5, h6{ margin-left: 20px; }\
\
	li > ul,\
	li > ol{ margin: 0; margin-left: 40px; }\
\
	dl > dd{ margin-left: 20px; }\
	\
	li > p { margin: 0; }\
\
p, li, dd, dt, pre{ line-height: 1.5; }\
\
table {\
	border-collapse: collapse;\
    margin-bottom: 20px;\
	margin-left:20px;\
	}\
\
th {\
	padding: 0 0.5em;\
	text-align: center;\
	}\
\
th {\
	border: 1px solid #FB7A31;\
	background: #FFC;\
	}\
\
td {\
	border-bottom: 1px solid #CCC;\
	border-right: 1px solid #CCC;\
	border-left: 1px solid #CCC;\
	padding: 0 0.5em;\
	}\
\
\
a:link,\
a:visited{ color: #1A5B8D; }\
\
a:hover,\
a:active{ color: #742CAC; }\
\
a.headerlink{ visibility: hidden; }\
\
	:hover > a.headerlink { visibility: visible; }\
\
a img{ \
	border: 0;\
	outline: 0;\
}\
\
img{ display: block; max-width: 100%; }\
\
code {\
	border: 1px solid #FB7A31;\
	background: #FFC;\
}\
\
pre {\
 white-space: pre-wrap;\
 white-space: -moz-pre-wrap;\
 white-space: -o-pre-wrap;\
border: 1px solid #FB7A31;\
background: #FFC;\
padding:5px;\
padding-left: 15px;\
}\
\
  </style>\
  </head><body><h1>Spectrum 2 web interface</h1>";
}


static void get_qsvar(const struct mg_request_info *request_info,
                      const char *name, char *dst, size_t dst_len) {
  const char *qs = request_info->query_string;
  mg_get_var(qs, strlen(qs == NULL ? "" : qs), name, dst, dst_len);
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
  mg_md5(buf, random, user, NULL);
}

Server::Server(ManagerConfig *config) {
	srand((unsigned) time(0));
	m_config = config;
	m_user = CONFIG_STRING(m_config, "service.admin_username");
	m_password = CONFIG_STRING(m_config, "service.admin_password");
}

Server::~Server() {
	mg_stop(ctx);
}


static void *_event_handler(enum mg_event event, struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);
	return static_cast<Server *>(request_info->user_data)->event_handler(event, conn);
}

bool Server::start() {
	const char *options[] = {
		"listening_ports", boost::lexical_cast<std::string>(CONFIG_INT(m_config, "service.port")).c_str(),
		"num_threads", "1",
		NULL
	};

	// Setup and start Mongoose
	if ((ctx = mg_start(&_event_handler, this, options)) == NULL) {
		return false;
	}

	return true;
}

bool Server::check_password(const char *user, const char *password) {
	return (m_user == user && m_password == password);
}

// Allocate new session object
Server::session *Server::new_session(const char *user) {
	Server::session *session = new Server::session;

	my_strlcpy(session->user, user, sizeof(session->user));
	snprintf(session->random, sizeof(session->random), "%d", rand());
	generate_session_id(session->session_id, session->random, session->user);
	session->expire = time(0) + SESSION_TTL;

	sessions[session->session_id] = session;
	return session;
}

// Get session object for the connection. Caller must hold the lock.
Server::session *Server::get_session(const struct mg_connection *conn) {
	time_t now = time(NULL);
	char session_id[33];
	mg_get_cookie(conn, "session", session_id, sizeof(session_id));

	if (sessions.find(session_id) == sessions.end()) {
		return NULL;
	}

	if (sessions[session_id]->expire != 0 && sessions[session_id]->expire > now) {
		return sessions[session_id];
	}

	return NULL;
}

void Server::authorize(struct mg_connection *conn, const struct mg_request_info *request_info) {
	char user[255], password[255];
	Server::session *session;

	// Fetch user name and password.
	get_qsvar(request_info, "user", user, sizeof(user));
	get_qsvar(request_info, "password", password, sizeof(password));

	if (check_password(user, password) && (session = new_session(user)) != NULL) {
		std::cout << "User authorized\n";
		// Authentication success:
		//   1. create new session
		//   2. set session ID token in the cookie
		//   3. remove original_url from the cookie - not needed anymore
		//   4. redirect client back to the original URL
		//
		// The most secure way is to stay HTTPS all the time. However, just to
		// show the technique, we redirect to HTTP after the successful
		// authentication. The danger of doing this is that session cookie can
		// be stolen and an attacker may impersonate the user.
		// Secure application must use HTTPS all the time.
		mg_printf(conn, "HTTP/1.1 302 Found\r\n"
			"Set-Cookie: session=%s; max-age=3600; http-only\r\n"  // Session ID
			"Set-Cookie: user=%s\r\n"  // Set user, needed by Javascript code
			"Set-Cookie: original_url=/; max-age=0\r\n"  // Delete original_url
			"Location: /\r\n\r\n",
			session->session_id, session->user);
	} else {
		// Authentication failure, redirect to login.
		redirect_to(conn, request_info, "/login");
	}
}

bool Server::is_authorized(const struct mg_connection *conn, const struct mg_request_info *request_info) {
	Server::session *session;
	char valid_id[33];
	bool authorized = false;

	// Always authorize accesses to login page and to authorize URI
	if (!strcmp(request_info->uri, "/login") ||
		!strcmp(request_info->uri, "/authorize")) {
		return true;
	}

// 	pthread_rwlock_rdlock(&rwlock);
	if ((session = get_session(conn)) != NULL) {
		generate_session_id(valid_id, session->random, session->user);
		if (strcmp(valid_id, session->session_id) == 0) {
		session->expire = time(0) + SESSION_TTL;
		authorized = true;
		}
	}
// 	pthread_rwlock_unlock(&rwlock);

	return authorized;
}

void Server::redirect_to(struct mg_connection *conn, const struct mg_request_info *request_info, const char *where) {
	mg_printf(conn, "HTTP/1.1 302 Found\r\n"
		"Set-Cookie: original_url=%s\r\n"
		"Location: %s\r\n\r\n",
		request_info->uri, where);
}

void Server::print_html(struct mg_connection *conn, const struct mg_request_info *request_info, const std::string &html) {
	mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %d\r\n"        // Always set Content-Length
			"\r\n"
			"%s",
			(int) html.size(), html.c_str());
}

void Server::serve_login(struct mg_connection *conn, const struct mg_request_info *request_info) {
	std::string html= "\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\
  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\"> \
<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" dir=\"ltr\"> \
  <head>\
    <title>Spectrum 2 web interface</title>\
    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"/>\
  </head>\
  <body>\
    <center>\
      <h2>Spectrum 2 web interface login</h2>\
      <br/>\
      <form action=\"/authorize\">\
        Username: <input type=\"text\" name=\"user\"></input><br/>\
        Password: <input type=\"password\" name=\"password\"></input><br/>\
        <input type=\"submit\" value=\"Login\"></input>\
      </form>\
    </center>\
  </body>\
</html>";

	print_html(conn, request_info, html);
}

void Server::serve_onlineusers(struct mg_connection *conn, const struct mg_request_info *request_info) {
	std::string html = get_header();
	char jid[255];
	get_qsvar(request_info, "jid", jid, sizeof(jid));

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
	print_html(conn, request_info, html);
}

void Server::serve_cmd(struct mg_connection *conn, const struct mg_request_info *request_info) {
	std::string html = get_header();
	char jid[255];
	get_qsvar(request_info, "jid", jid, sizeof(jid));
	char cmd[4096];
	get_qsvar(request_info, "cmd", cmd, sizeof(cmd));

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
	print_html(conn, request_info, html);
}


void Server::serve_start(struct mg_connection *conn, const struct mg_request_info *request_info) {
	std::string html= get_header() ;
	char jid[255];
	get_qsvar(request_info, "jid", jid, sizeof(jid));

	start_instances(m_config, jid);
	html += "<b>" + get_response() + "</b><br/><a href=\"/\">Back to main page</a>";
	html += "</body></html>";
	print_html(conn, request_info, html);
}

void Server::serve_stop(struct mg_connection *conn, const struct mg_request_info *request_info) {
	std::string html= get_header();
	char jid[255];
	get_qsvar(request_info, "jid", jid, sizeof(jid));

	stop_instances(m_config, jid);
	html += "<b>" + get_response() + "</b><br/><a href=\"/\">Back to main page</a>";
	html += "</body></html>";
	print_html(conn, request_info, html);
}

void Server::serve_root(struct mg_connection *conn, const struct mg_request_info *request_info) {
	std::vector<std::string> list = show_list(m_config, false);
	std::string html= get_header() + "<h2>List of instances</h2><table><tr><th>JID<th>Status</th><th>Command</th><th>Run command</th></tr>";

	BOOST_FOREACH(std::string &instance, list) {
		html += "<tr>";
		html += "<td><a href=\"/onlineusers?jid=" + instance + "\">" + instance + "</a></td>";
		Swift::SimpleEventLoop eventLoop;
		Swift::BoostNetworkFactories networkFactories(&eventLoop);

		ask_local_server(m_config, networkFactories, instance, "status");
		eventLoop.runUntilEvents();
		while(get_response().empty()) {
			eventLoop.runUntilEvents();
		}
		html += "<td>" + get_response() + "</td>";
		if (get_response().find("Running") == 0) {
			html += "<td><a href=\"/stop?jid=" + instance + "\">Stop</a></td>";
			html += "<td><form action=\"/cmd\">";
			html += "<input type=\"hidden\" name=\"jid\" value=\"" + instance + "\"></input>";
			html += "<input type=\"text\" name=\"cmd\"></input>";
			html += "<input type=\"submit\" value=\"Run\"></input>";
			html += "</form></td>";
		}
		else {
			html += "<td><a href=\"/start?jid=" + instance + "\">Start</a></td>";
			html += "<td></td>";
		}

		html += "</tr>";
	}

	html += "</table></body></html>";
	print_html(conn, request_info, html);
}

void *Server::event_handler(enum mg_event event, struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);
	void *processed = (void *) 0x1;

	if (event == MG_NEW_REQUEST) {
		if (!is_authorized(conn, request_info)) {
			redirect_to(conn, request_info, "/login");
		} else if (strcmp(request_info->uri, "/authorize") == 0) {
			authorize(conn, request_info);
		} else if (strcmp(request_info->uri, "/login") == 0) {
			serve_login(conn, request_info);
		} else if (strcmp(request_info->uri, "/") == 0) {
			serve_root(conn, request_info);
		} else if (strcmp(request_info->uri, "/onlineusers") == 0) {
			serve_onlineusers(conn, request_info);
		} else if (strcmp(request_info->uri, "/cmd") == 0) {
			serve_cmd(conn, request_info);
		} else if (strcmp(request_info->uri, "/start") == 0) {
			serve_start(conn, request_info);
		} else if (strcmp(request_info->uri, "/stop") == 0) {
			serve_stop(conn, request_info);
		} else {
			// No suitable handler found, mark as not processed. Mongoose will
			// try to serve the request.
			processed = NULL;
		}
	} else {
		processed = NULL;
	}

	return processed;
}




