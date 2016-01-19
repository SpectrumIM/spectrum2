/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2016, Jan Kaluza <hanzz.k@gmail.com>
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

#include "rapidjson/document.h"

#include "mongoose.h"
#include "managerconfig.h"
#include "server.h"

#include "transport/Config.h"
#include "transport/SQLite3Backend.h"
#include "transport/MySQLBackend.h"
#include "transport/PQXXBackend.h"
#include "transport/StorageBackend.h"

using namespace Transport;
using namespace rapidjson;

class APIServer {
	public:
		APIServer(ManagerConfig *config, StorageBackend *storage);

		virtual ~APIServer();

		void handleRequest(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm);

	private:
		void serve_instances(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm);
		void serve_instances_start(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm);
		void serve_instances_stop(Server *server, Server::session *sess, struct mg_connection *conn, struct http_message *hm);
		void send_json(struct mg_connection *conn, const Document &d);
		void send_ack(struct mg_connection *conn, bool error, const std::string &message);

	private:
		ManagerConfig *m_config;
		StorageBackend *m_storage;
};
