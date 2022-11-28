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

#include "managerconfig.h"
#include <fstream>

using namespace boost::program_options;

bool ManagerConfig::load(const std::string &configfile, boost::program_options::options_description &opts) {
	std::ifstream ifs(configfile.c_str());
	if (!ifs.is_open())
		return false;

	opts.add_options()
		("service.admin_username", value<std::string>()->default_value(""), "Administrator username.")
		("service.admin_password", value<std::string>()->default_value(""), "Administrator password.")
		("service.bind", value<std::string>()->default_value("localhost"), "IP to bind. Empty: binds all interfaces.")
		("service.port", value<int>()->default_value(8081), "Web interface port.")
		("service.cert", value<std::string>()->default_value(""), "Web interface certificate in PEM format when TLS should be used.")
		("service.config_directory", value<std::string>()->default_value("/etc/spectrum2/transports/"), "Directory with spectrum2 configuration files. One .cfg file per one instance")
		("service.data_dir", value<std::string>()->default_value("/var/lib/spectrum2_manager/html"), "Directory to store Spectrum 2 manager data")
		("service.base_location", value<std::string>()->default_value("/"), "Base location of the web interface")
		("servers.server", value<std::vector<std::string> >()->multitoken(), "Server.")
		("database.type", value<std::string>()->default_value("none"), "Database type.")
		("database.database", value<std::string>()->default_value("/var/lib/spectrum2/$jid/database.sql"), "Database used to store data")
		("database.server", value<std::string>()->default_value("localhost"), "Database server.")
		("database.user", value<std::string>()->default_value(""), "Database user.")
		("database.password", value<std::string>()->default_value(""), "Database Password.")
		("database.port", value<int>()->default_value(0), "Database port.")
		("database.prefix", value<std::string>()->default_value(""), "Prefix of tables in database")
		("logging.config", value<std::string>()->default_value("/etc/spectrum2/manager_logging.cfg"), "Logging configuration file")
	;

	store(parse_config_file(ifs, opts), m_variables);
	notify(m_variables);

	m_file = configfile;

	onManagerConfigReloaded();

	return true;
}

bool ManagerConfig::load(const std::string &configfile) {
	options_description opts("Transport options");
	return load(configfile, opts);
}
