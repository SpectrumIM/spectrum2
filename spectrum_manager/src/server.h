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
#include <boost/signal.hpp>

#include "mongoose.h"
#include "managerconfig.h"

class Server {
	public:
		struct session {
			char session_id[33];      // Session ID, must be unique
			char random[20];          // Random data used for extra user validation
			char user[255];  // Authenticated user
			time_t expire;            // Expiration timestamp, UTC
		};

		/// Constructor.
		Server(ManagerConfig *config);

		/// Destructor
		virtual ~Server();

		bool start();

		void *event_handler(enum mg_event event, struct mg_connection *conn);

	private:
		void serve_login(struct mg_connection *conn, const struct mg_request_info *request_info);
		void serve_root(struct mg_connection *conn, const struct mg_request_info *request_info);
		void serve_start(struct mg_connection *conn, const struct mg_request_info *request_info);
		void serve_stop(struct mg_connection *conn, const struct mg_request_info *request_info);
		void serve_onlineusers(struct mg_connection *conn, const struct mg_request_info *request_info);
		void serve_cmd(struct mg_connection *conn, const struct mg_request_info *request_info);
		void print_html(struct mg_connection *conn, const struct mg_request_info *request_info, const std::string &html);

	private:
		bool check_password(const char *user, const char *password);
		session *new_session(const char *user);
		session *get_session(const struct mg_connection *conn);

		void authorize(struct mg_connection *conn, const struct mg_request_info *request_info);

		bool is_authorized(const struct mg_connection *conn, const struct mg_request_info *request_info);

		void redirect_to(struct mg_connection *conn, const struct mg_request_info *request_info, const char *where);

	private:
		struct mg_context *ctx;
		std::map<std::string, session *> sessions;
		std::string m_user;
		std::string m_password;
		ManagerConfig *m_config;
};
