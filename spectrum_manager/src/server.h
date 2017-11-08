/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#pragma once

#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>

#include "mongoose.h"
#include "managerconfig.h"

#include "transport/Config.h"
#include "transport/Logging.h"
#include "transport/SQLite3Backend.h"
#include "transport/MySQLBackend.h"
#include "transport/PQXXBackend.h"
#include "transport/StorageBackend.h"

using namespace Transport;

class APIServer;

class Server {
	public:
		struct session {
			char session_id[33];      // Session ID, must be unique
			char random[20];          // Random data used for extra user validation
			char user[255];  // Authenticated user
			time_t expire;            // Expiration timestamp, UTC
			bool admin;
		};

		/// Constructor.
		Server(ManagerConfig *config, const std::string &config_file);

		/// Destructor
		virtual ~Server();

		bool start();

		void event_handler(struct mg_connection *nc, int ev, void *p);

		void redirect_to(struct mg_connection *conn, struct http_message *hm, const char *where);

		std::string send_command(const std::string &jid, const std::string &cmd, int timeout = 1);

	private:
		void serve_logout(struct mg_connection *conn, struct http_message *hm);
		void serve_oauth2(struct mg_connection *conn, struct http_message *hm);
		void print_html(struct mg_connection *conn, struct http_message *hm, const std::string &html);

	private:
		bool check_password(const std::string &user, const std::string &password);
		session *new_session(const std::string &user);
		session *get_session(struct http_message *hm);

		void authorize(struct mg_connection *conn, struct http_message *hm);

		bool is_authorized(const struct mg_connection *conn, struct http_message *hm);

	private:
		struct mg_mgr m_mgr;
		struct mg_connection *m_nc;

		std::map<std::string, session *> sessions;
		std::string m_user;
		std::string m_password;
		ManagerConfig *m_config;
		std::string m_header;
		std::string m_footer;
		Config *m_storageCfg;
		StorageBackend *m_storage;
		APIServer *m_apiServer;
};
