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
#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#include <windows.h>
#define PATH_MAX MAX_PATH
#endif

using namespace boost::program_options;

namespace Transport {
int getRandomPort(const std::string &s) {
	unsigned long r = 0;
	BOOST_FOREACH(char c, s) {
		r += (int) c;
	}
	srand(time(NULL) + r);
	return 30000 + rand() % 10000;
}

bool Config::load(const std::string &configfile, boost::program_options::options_description &opts, const std::string &jid) {
	std::ifstream ifs(configfile.c_str());
	if (!ifs.is_open())
		return false;

	m_file = configfile;
	bool ret = load(ifs, opts, jid);
	ifs.close();

	char path[PATH_MAX] = "";
	if (m_file.find_first_of("/") != 0) {
		getcwd(path, PATH_MAX);
		m_file = std::string(path) + "/" + m_file;
	}

	return ret;
}

bool Config::load(std::istream &ifs, boost::program_options::options_description &opts, const std::string &_jid) {
	m_unregistered.clear();
	opts.add_options()
		("service.jid", value<std::string>()->default_value(""), "Transport Jabber ID")
		("service.server", value<std::string>()->default_value(""), "Server to connect to")
		("service.password", value<std::string>()->default_value(""), "Password used to auth the server")
		("service.port", value<int>()->default_value(0), "Port the server is listening on")
		("service.user", value<std::string>()->default_value(""), "The name of user Spectrum runs as.")
		("service.group", value<std::string>()->default_value(""), "The name of group Spectrum runs as.")
		("service.backend", value<std::string>()->default_value("libpurple_backend"), "Backend")
		("service.protocol", value<std::string>()->default_value(""), "Protocol")
		("service.pidfile", value<std::string>()->default_value("/var/run/spectrum2/$jid.pid"), "Full path to pid file")
		("service.working_dir", value<std::string>()->default_value("/var/lib/spectrum2/$jid"), "Working dir")
		("service.allowed_servers", value<std::string>()->default_value(""), "Only users from these servers can connect")
		("service.server_mode", value<bool>()->default_value(false), "True if Spectrum should behave as server")
		("service.users_per_backend", value<int>()->default_value(100), "Number of users per one legacy network backend")
		("service.backend_host", value<std::string>()->default_value("localhost"), "Host to bind backend server to")
		("service.backend_port", value<std::string>()->default_value("0"), "Port to bind backend server to")
		("service.cert", value<std::string>()->default_value(""), "PKCS#12 Certificate.")
		("service.cert_password", value<std::string>()->default_value(""), "PKCS#12 Certificate password.")
		("service.admin_jid", value<std::string>()->default_value(""), "Administrator jid.")
		("service.admin_password", value<std::string>()->default_value(""), "Administrator password.")
		("service.reuse_old_backends", value<bool>()->default_value(true), "True if Spectrum should use old backends which were full in the past.")
		("service.idle_reconnect_time", value<int>()->default_value(0), "Time in seconds after which idle users are reconnected to let their backend die.")
		("service.memory_collector_time", value<int>()->default_value(0), "Time in seconds after which backend with most memory is set to die.")
		("service.more_resources", value<bool>()->default_value(false), "Allow more resources to be connected in server mode at the same time.")
		("service.enable_privacy_lists", value<bool>()->default_value(true), "")
		("vhosts.vhost", value<std::vector<std::string> >()->multitoken(), "")
		("identity.name", value<std::string>()->default_value("Spectrum 2 Transport"), "Name showed in service discovery.")
		("identity.category", value<std::string>()->default_value("gateway"), "Disco#info identity category. 'gateway' by default.")
		("identity.type", value<std::string>()->default_value(""), "Type of transport ('icq','msn','gg','irc', ...)")
		("registration.enable_public_registration", value<bool>()->default_value(true), "True if users should be able to register.")
		("registration.language", value<std::string>()->default_value("en"), "Default language for registration form")
		("registration.instructions", value<std::string>()->default_value("Enter your legacy network username and password."), "Instructions showed to user in registration form")
		("registration.username_label", value<std::string>()->default_value("Legacy network username:"), "Label for username field")
		("registration.username_mask", value<std::string>()->default_value(""), "Username mask")
		("registration.auto_register", value<bool>()->default_value(false), "Register new user automatically when the presence arrives.")
		("registration.encoding", value<std::string>()->default_value("utf8"), "Default encoding in registration form")
		("registration.require_local_account", value<bool>()->default_value(false), "True if users have to have a local account to register to this transport from remote servers.")
		("registration.local_username_label", value<std::string>()->default_value("Local username:"), "Label for local usernme field")
		("registration.local_account_server", value<std::string>()->default_value("localhost"), "The server on which the local accounts will be checked for validity")
		("registration.local_account_server_timeout", value<int>()->default_value(10000), "Timeout when checking local user on local_account_server (msecs)")
		("gateway_responder.prompt", value<std::string>()->default_value("Contact ID"), "Value of <prompt> </promt> field")
		("gateway_responder.label", value<std::string>()->default_value("Enter legacy network contact ID."), "Label for add contact ID field")
		("database.type", value<std::string>()->default_value("none"), "Database type.")
		("database.database", value<std::string>()->default_value("/var/lib/spectrum2/$jid/database.sql"), "Database used to store data")
		("database.server", value<std::string>()->default_value("localhost"), "Database server.")
		("database.user", value<std::string>()->default_value(""), "Database user.")
		("database.password", value<std::string>()->default_value(""), "Database Password.")
		("database.port", value<int>()->default_value(0), "Database port.")
		("database.prefix", value<std::string>()->default_value(""), "Prefix of tables in database")
		("database.encryption_key", value<std::string>()->default_value(""), "Encryption key.")
		("logging.config", value<std::string>()->default_value(""), "Path to log4cxx config file which is used for Spectrum 2 instance")
		("logging.backend_config", value<std::string>()->default_value(""), "Path to log4cxx config file which is used for backends")
		("backend.default_avatar", value<std::string>()->default_value(""), "Full path to default avatar")
		("backend.avatars_directory", value<std::string>()->default_value(""), "Path to directory with avatars")
		("backend.no_vcard_fetch", value<bool>()->default_value(false), "True if VCards for buddies should not be fetched. Only avatars will be forwarded.")
	;

	parsed_options parsed = parse_config_file(ifs, opts, true);

	bool found_working = false;
	bool found_pidfile = false;
	bool found_backend_port = false;
	bool found_database = false;
	std::string jid = "";
	BOOST_FOREACH(option &opt, parsed.options) {
		if (opt.string_key == "service.jid") {
			if (_jid.empty()) {
				jid = opt.value[0];
			}
			else {
				opt.value[0] = _jid;
				jid = _jid;
			}
		}
		else if (opt.string_key == "service.backend_port") {
			found_backend_port = true;
			if (opt.value[0] == "0") {
				opt.value[0] = boost::lexical_cast<std::string>(getRandomPort(_jid.empty() ? jid : _jid));
			}
		}
		else if (opt.string_key == "service.working_dir") {
			found_working = true;
		}
		else if (opt.string_key == "service.pidfile") {
			found_pidfile = true;
		}
		else if (opt.string_key == "database.database") {
			found_database = true;
		}
	}

	if (!found_working) {
		std::vector<std::string> value;
		value.push_back("/var/lib/spectrum2/$jid");
		parsed.options.push_back(boost::program_options::basic_option<char>("service.working_dir", value));
	}
	if (!found_pidfile) {
		std::vector<std::string> value;
		value.push_back("/var/run/spectrum2/$jid.pid");
		parsed.options.push_back(boost::program_options::basic_option<char>("service.pidfile", value));
	}
	if (!found_backend_port) {
		std::vector<std::string> value;
		std::string p = boost::lexical_cast<std::string>(getRandomPort(_jid.empty() ? jid : _jid));
		value.push_back(p);
		parsed.options.push_back(boost::program_options::basic_option<char>("service.backend_port", value));
	}
	if (!found_database) {
		std::vector<std::string> value;
		value.push_back("/var/lib/spectrum2/$jid/database.sql");
		parsed.options.push_back(boost::program_options::basic_option<char>("database.database", value));
	}

	BOOST_FOREACH(option &opt, parsed.options) {
		if (opt.unregistered) {
			m_unregistered[opt.string_key] = opt.value[0];
		}
		else if (opt.value[0].find("$jid") != std::string::npos) {
			boost::replace_all(opt.value[0], "$jid", jid);
		}
	}

	store(parsed, m_variables);
	notify(m_variables);

	onConfigReloaded();

	return true;
}

bool Config::load(std::istream &ifs) {
	options_description opts("Transport options");
	return load(ifs, opts);
}

bool Config::load(const std::string &configfile, const std::string &jid) {
	try {
		options_description opts("Transport options");
		return load(configfile, opts, jid);
	} catch ( const boost::program_options::multiple_occurrences& e ) {
		std::cerr << configfile << " parsing error: " << e.what() << " from option: " << e.get_option_name() << std::endl;
		return false;
	}
}

bool Config::reload() {
	if (m_file.empty()) {
		return false;
	}

	return load(m_file);
}

}
