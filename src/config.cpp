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

#include "transport/config.h"
#include <fstream>

using namespace boost::program_options;

namespace Transport {

bool Config::load(const std::string &configfile, boost::program_options::options_description &opts) {
	std::ifstream ifs(configfile.c_str());
	if (!ifs.is_open())
		return false;

	opts.add_options()
		("service.jid", value<std::string>()->default_value(""), "Transport Jabber ID")
		("service.server", value<std::string>()->default_value(""), "Server to connect to")
		("service.password", value<std::string>()->default_value(""), "Password used to auth the server")
		("service.port", value<int>()->default_value(0), "Port the server is listening on")
		("service.backend", value<std::string>()->default_value("libpurple_backend"), "Backend")
		("service.protocol", value<std::string>()->default_value(""), "Protocol")
		("service.allowed_servers", value<std::string>()->default_value(""), "Only users from these servers can connect")
		("service.server_mode", value<bool>()->default_value(false), "True if Spectrum should behave as server")
		("service.users_per_backend", value<int>()->default_value(100), "Number of users per one legacy network backend")
		("service.backend_host", value<std::string>()->default_value("localhost"), "Host to bind backend server to")
		("service.backend_port", value<std::string>()->default_value("10000"), "Port to bind backend server to")
		("service.cert", value<std::string>()->default_value(""), "PKCS#12 Certificate.")
		("service.cert_password", value<std::string>()->default_value(""), "PKCS#12 Certificate password.")
		("registration.enable_public_registration", value<bool>()->default_value(true), "True if users should be able to register.")
		("registration.language", value<std::string>()->default_value("en"), "Default language for registration form")
		("registration.instructions", value<std::string>()->default_value(""), "Instructions showed to user in registration form")
		("registration.username_field", value<std::string>()->default_value(""), "Label for username field")
		("registration.username_mask", value<std::string>()->default_value(""), "Username mask")
		("registration.encoding", value<std::string>()->default_value("en"), "Default encoding in registration form")
		("database.type", value<std::string>()->default_value("none"), "Database type.")
		("database.database", value<std::string>()->default_value(""), "Database used to store data")
		("database.prefix", value<std::string>()->default_value(""), "Prefix of tables in database")
	;

    store(parse_config_file(ifs, opts), m_variables);
	notify(m_variables);

	m_file = configfile;

	onConfigReloaded();

	return true;
}

bool Config::load(const std::string &configfile) {
	options_description opts("Transport options");
	return load(configfile, opts);
}

}
