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

#include "transport/Config.h"
#include "transport/CommandLine.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <list>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#include <windows.h>
#ifdef _MSC_VER
#define PATH_MAX MAX_PATH
#endif
#else
#include <limits.h>
#include <unistd.h>
#endif

#include "iostream"
#include "inih/ini.h"
#include "transport/Util.h"

namespace Transport {

namespace {

enum OptionType { OPTION_STRING, OPTION_INT, OPTION_BOOL, OPTION_VECTOR };

struct OptionDesc {
	const char *key;
	OptionType type;
	// NULL for options without a default (OPTION_VECTOR options)
	const char *defaultValue;
};

const OptionDesc transportOptions[] = {
	{"service.jid", OPTION_STRING, ""},
	{"service.server", OPTION_STRING, "127.0.0.1"},
	{"service.password", OPTION_STRING, ""},
	{"service.port", OPTION_INT, "0"},
	{"service.user", OPTION_STRING, ""},
	{"service.group", OPTION_STRING, ""},
	{"service.backend", OPTION_STRING, "libpurple_backend"},
	{"service.protocol", OPTION_STRING, ""},
	{"service.pidfile", OPTION_STRING, "/var/run/spectrum2/$jid.pid"},
	{"service.portfile", OPTION_STRING, "/var/run/spectrum2/$jid.port"},
	{"service.working_dir", OPTION_STRING, "/var/lib/spectrum2/$jid"},
	{"service.allowed_servers", OPTION_VECTOR, NULL},
	{"service.server_mode", OPTION_BOOL, "false"},
	{"service.users_per_backend", OPTION_INT, "100"},
	{"service.backend_host", OPTION_STRING, "127.0.0.1"},
	{"service.backend_port", OPTION_STRING, "0"},
	{"service.cert", OPTION_STRING, ""},
	{"service.cert_password", OPTION_STRING, ""},
	{"service.admin_jid", OPTION_VECTOR, NULL},
	{"service.admin_password", OPTION_STRING, ""},
	{"service.reuse_old_backends", OPTION_BOOL, "true"},
	{"service.idle_reconnect_time", OPTION_INT, "0"},
	{"service.memory_collector_time", OPTION_INT, "0"},
	{"service.more_resources", OPTION_BOOL, "false"},
	{"service.enable_privacy_lists", OPTION_BOOL, "true"},
	{"service.enable_xhtml", OPTION_BOOL, "true"},
	{"service.enable_remove_buddy", OPTION_BOOL, "false"},
	{"service.oob_replace_body", OPTION_BOOL, "true"},
	{"service.oob_split", OPTION_BOOL, "false"},
	{"service.max_room_list_size", OPTION_INT, "100"},
	{"service.login_delay", OPTION_INT, "0"},
	{"service.jid_escaping", OPTION_BOOL, "true"},
	{"service.vip_only", OPTION_BOOL, "false"},
	{"service.vip_message", OPTION_STRING, ""},
	{"service.reconnect_all_users", OPTION_BOOL, "false"},
	{"service.frontend", OPTION_STRING, "xmpp"},
	{"service.web_directory", OPTION_STRING, ""},
	{"service.web_url", OPTION_STRING, ""},
	{"service.web_maximgsize", OPTION_INT, "1000000"},
	{"vhosts.vhost", OPTION_VECTOR, NULL},
	{"identity.name", OPTION_STRING, "Spectrum 2 Transport"},
	{"identity.category", OPTION_STRING, "gateway"},
	{"identity.type", OPTION_STRING, ""},
	{"registration.enable_public_registration", OPTION_BOOL, "true"},
	{"registration.language", OPTION_STRING, "en"},
	{"registration.instructions", OPTION_STRING, "Enter your legacy network username and password."},
	{"registration.username_label", OPTION_STRING, "3rd-party network username"},
	{"registration.password_label", OPTION_STRING, "3rd-party network password"},
	{"registration.username_mask", OPTION_STRING, ""},
	{"registration.allowed_usernames", OPTION_STRING, ""},
	{"registration.auto_register", OPTION_BOOL, "false"},
	{"registration.encoding", OPTION_STRING, "utf8"},
	{"registration.require_local_account", OPTION_BOOL, "false"},
	{"registration.notify_jid", OPTION_VECTOR, NULL},
	{"registration.local_username_label", OPTION_STRING, "Local username:"},
	{"registration.local_account_server", OPTION_STRING, "localhost"},
	{"registration.local_account_server_timeout", OPTION_INT, "10000"},
	{"gateway_responder.prompt", OPTION_STRING, "Contact ID"},
	{"gateway_responder.label", OPTION_STRING, "Enter legacy network contact ID."},
	{"database.type", OPTION_STRING, "none"},
	{"database.database", OPTION_STRING, "/var/lib/spectrum2/$jid/database.sql"},
	{"database.server", OPTION_STRING, "localhost"},
	{"database.user", OPTION_STRING, ""},
	{"database.password", OPTION_STRING, ""},
	{"database.port", OPTION_INT, "0"},
	{"database.prefix", OPTION_STRING, ""},
	{"database.encryption_key", OPTION_STRING, ""},
	{"database.vip_statement", OPTION_STRING, ""},
	{"logging.config", OPTION_STRING, ""},
	{"logging.backend_config", OPTION_STRING, ""},
	{"backend.default_avatar", OPTION_STRING, ""},
	{"backend.avatars_directory", OPTION_STRING, ""},
	{"backend.no_vcard_fetch", OPTION_BOOL, "false"},
	{"proxy.server", OPTION_STRING, "localhost"},
	{"proxy.user", OPTION_STRING, ""},
	{"proxy.password", OPTION_STRING, ""},
	{"proxy.port", OPTION_INT, "0"},
	{NULL, OPTION_STRING, NULL}
};

const OptionDesc backendOptions[] = {
	{"registration.needPassword", OPTION_BOOL, "true"},
	{"registration.needRegistration", OPTION_BOOL, "true"},
	{"registration.extraField", OPTION_VECTOR, NULL},
	{"features.receipts", OPTION_BOOL, "false"},
	{"features.muc", OPTION_BOOL, "false"},
	{"features.rawxml", OPTION_BOOL, "false"},
	{"features.disable_jid_escaping", OPTION_BOOL, "false"},
	{"features.send_buddies_on_login", OPTION_BOOL, "false"},
	{NULL, OPTION_STRING, NULL}
};

struct ParsedOption {
	std::string key;
	std::string value;
};

const OptionDesc *findOption(const OptionDesc *options, const std::string &key) {
	for (const OptionDesc *desc = options; desc->key; desc++) {
		if (key == desc->key) {
			return desc;
		}
	}
	return NULL;
}

int handleIniValue(void *user, const char *section, const char *name, const char *value) {
	std::vector<ParsedOption> *parsed = static_cast<std::vector<ParsedOption> *>(user);
	std::string key = *section ? std::string(section) + "." + name : std::string(name);
	std::string val(value);
	// '#' starts a comment anywhere on the line, as it did with boost::program_options
	size_t comment = val.find('#');
	if (comment != std::string::npos) {
		val.erase(comment);
		val.erase(val.find_last_not_of(" \t\r\n") + 1);
	}
	parsed->push_back({key, val});
	return 1;
}

// Parses "section.key = value" lines; returns false and sets error on a syntax error.
bool parseIni(const std::string &data, std::vector<ParsedOption> &parsed, std::string &error) {
	int line = ini_parse_string_length(data.c_str(), data.size(), handleIniValue, &parsed);
	if (line != 0) {
		error = "invalid line " + std::to_string(line);
		return false;
	}
	return true;
}

bool parseValue(const OptionDesc &desc, const std::string &raw, ConfigValue &value, std::string &error) {
	switch (desc.type) {
		case OPTION_INT: {
			size_t pos = 0;
			int i = 0;
			try {
				i = std::stoi(raw, &pos);
			}
			catch (...) {
				pos = 0;
			}
			if (raw.empty() || pos != raw.size()) {
				error = "the argument ('" + raw + "') for option '" + desc.key + "' is invalid";
				return false;
			}
			value = ConfigValue(i);
			return true;
		}
		case OPTION_BOOL: {
			std::string s(raw);
			std::transform(s.begin(), s.end(), s.begin(), ::tolower);
			if (s.empty() || s == "on" || s == "yes" || s == "1" || s == "true") {
				value = ConfigValue(true);
			}
			else if (s == "off" || s == "no" || s == "0" || s == "false") {
				value = ConfigValue(false);
			}
			else {
				error = "the argument ('" + raw + "') for option '" + desc.key + "' is invalid";
				return false;
			}
			return true;
		}
		default:
			value = ConfigValue(raw);
			return true;
	}
}

// Stores values of registered options. Variables in finalKeys already got their
// value from an earlier store (command line, previous load) and are kept;
// default values are replaced. Options not given anywhere get their default.
bool storeOptions(const OptionDesc *options, const std::vector<ParsedOption> &parsed, std::map<std::string, ConfigValue> &variables, std::set<std::string> &finalKeys, std::string &error) {
	std::set<std::string> stored;
	for (const ParsedOption &opt : parsed) {
		const OptionDesc *desc = findOption(options, opt.key);
		if (!desc || finalKeys.count(opt.key)) {
			continue;
		}

		if (desc->type == OPTION_VECTOR) {
			std::vector<std::string> values;
			if (stored.count(opt.key)) {
				values = variables[opt.key].as<std::vector<std::string> >();
			}
			values.push_back(opt.value);
			variables[opt.key] = ConfigValue(values);
		}
		else {
			if (stored.count(opt.key)) {
				error = "option '" + opt.key + "' cannot be specified more than once";
				return false;
			}
			if (!parseValue(*desc, opt.value, variables[opt.key], error)) {
				return false;
			}
		}
		stored.insert(opt.key);
	}
	finalKeys.insert(stored.begin(), stored.end());

	for (const OptionDesc *desc = options; desc->key; desc++) {
		if (desc->defaultValue && variables.find(desc->key) == variables.end()) {
			parseValue(*desc, desc->defaultValue, variables[desc->key], error);
		}
	}
	return true;
}

}

bool Config::load(const std::string &configfile, const std::string &jid) {
	std::ifstream ifs(configfile.c_str());
	if (!ifs.is_open())
		return false;

	m_file = configfile;
	m_jid = jid;
	bool ret = load(ifs, jid);
	ifs.close();
#ifndef WIN32
	char path[PATH_MAX] = "";
	if (m_file.find_first_of("/") != 0) {
		getcwd(path, PATH_MAX);
		m_file = std::string(path) + "/" + m_file;
	}
#endif

	return ret;
}

bool Config::load(std::istream &ifs, const std::string &_jid) {
	m_unregistered.clear();

	std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	std::vector<ParsedOption> parsed;
	std::string error;
	if (!parseIni(data, parsed, error)) {
		std::cerr << m_file << " parsing error: " << error << std::endl;
		return false;
	}

	bool found_working = false;
	bool found_pidfile = false;
	bool found_portfile = false;
	bool found_backend_port = false;
	bool found_database = false;
	std::string jid = "";
	for (ParsedOption &opt : parsed) {
		if (opt.key == "service.jid") {
			if (_jid.empty()) {
				jid = opt.value;
			}
			else {
				opt.value = _jid;
				jid = _jid;
			}
		}
		else if (opt.key == "service.backend_port") {
			found_backend_port = true;
			if (opt.value == "0") {
				opt.value = std::to_string(Util::getRandomPort(_jid.empty() ? jid : _jid));
			}
		}
		else if (opt.key == "service.working_dir") {
			found_working = true;
		}
		else if (opt.key == "service.pidfile") {
			found_pidfile = true;
		}
		else if (opt.key == "service.portfile") {
			found_portfile = true;
		}
		else if (opt.key == "database.database") {
			found_database = true;
		}
	}

	if (!found_working) {
		parsed.push_back({"service.working_dir", "/var/lib/spectrum2/$jid"});
	}
	if (!found_pidfile) {
		parsed.push_back({"service.pidfile", "/var/run/spectrum2/$jid.pid"});
	}
	if (!found_portfile) {
		parsed.push_back({"service.portfile", "/var/run/spectrum2/$jid.port"});
	}
	if (!found_backend_port) {
		parsed.push_back({"service.backend_port", std::to_string(Util::getRandomPort(_jid.empty() ? jid : _jid))});
	}
	if (!found_database) {
		parsed.push_back({"database.database", "/var/lib/spectrum2/$jid/database.sql"});
	}

	// Unregistered variables: a single value is a bool or a string,
	// a variable set more than once is a list of strings.
	std::map<std::string, std::list<std::string> > unregistered;
	for (ParsedOption &opt : parsed) {
		if (!findOption(transportOptions, opt.key)) {
			std::list<std::string> &values = unregistered[opt.key];
			values.push_back(opt.value);
			if (values.size() > 1) {
				m_unregistered[opt.key] = ConfigValue(values);
			}
			else if (opt.value == "true" || opt.value == "1") {
				m_unregistered[opt.key] = ConfigValue(true);
			}
			else if (opt.value == "false" || opt.value == "0") {
				m_unregistered[opt.key] = ConfigValue(false);
			}
			else {
				m_unregistered[opt.key] = ConfigValue(opt.value);
			}
		}
		else if (opt.value.find("$jid") != std::string::npos) {
			Util::replaceAll(opt.value, "$jid", jid);
		}
	}

	// Load configs passed by command line: "--section.key=value", or
	// "--section.key value" for registered variables.
	if (m_argc != 0 && m_argv) {
		std::vector<ParsedOption> cmdline;
		for (int i = 1; i < m_argc; i++) {
			std::string arg(m_argv[i]);
			if (arg.size() <= 2 || arg.compare(0, 2, "--") != 0) {
				continue;
			}
			std::string key = arg.substr(2);
			size_t eq = key.find('=');
			bool registered = findOption(transportOptions, key.substr(0, eq)) != NULL;
			if (eq != std::string::npos) {
				std::string value = key.substr(eq + 1);
				key = key.substr(0, eq);
				if (registered) {
					cmdline.push_back({key, value});
				}
				else if (!value.empty()) {
					m_unregistered[key] = ConfigValue(value);
				}
			}
			else if (registered && i + 1 < m_argc) {
				cmdline.push_back({key, m_argv[++i]});
			}
		}
		if (!storeOptions(transportOptions, cmdline, m_variables, m_final, error)) {
			std::cerr << "Command line parsing error: " << error << std::endl;
			return false;
		}
	}

	if (!storeOptions(transportOptions, parsed, m_variables, m_final, error)) {
		std::cerr << m_file << " parsing error: " << error << std::endl;
		return false;
	}

	if (onConfigReloaded) {
		onConfigReloaded();
	}

	return true;
}

bool Config::reload() {
	if (m_file.empty()) {
		return false;
	}

	return load(m_file, m_jid);
}

Config::SectionValuesCont Config::getSectionValues(const std::string& sectionName) {
	SectionValuesCont sectionValues;

	std::string sectionSearchString = sectionName + ".";
	for (const std::map<std::string, ConfigValue>::value_type &varItem : m_variables) {
		if (Util::iStartsWith(varItem.first, sectionSearchString))
			sectionValues[varItem.first] = varItem.second;
	}

	for (const UnregisteredCont::value_type &varItem : m_unregistered) {
		if (Util::iStartsWith(varItem.first, sectionSearchString))
			sectionValues[varItem.first] = varItem.second;
	}

	return sectionValues;
}

std::string Config::getCommandLineArgs() const {
	std::ostringstream commandLineArgs;

	// Return the command-line arguments that were passed to us originally (but remove the initial .exe part)
	for (int i = 1; i < m_argc; ++i) 	{
		commandLineArgs << "\"" << m_argv[i] << "\" ";
	}

	return commandLineArgs.str();
}

void Config::updateBackendConfig(const std::string &backendConfig) {
	std::vector<ParsedOption> parsed;
	std::string error;
	if (!parseIni(backendConfig, parsed, error) || !storeOptions(backendOptions, parsed, m_backendConfig, m_backendFinal, error)) {
		std::cerr << "Backend config parsing error: " << error << std::endl;
	}

	if (onBackendConfigUpdated) {
		onBackendConfigUpdated();
	}

	if (CONFIG_BOOL_DEFAULTED(this, "features.disable_jid_escaping", false)) {
		std::map<std::string, ConfigValue>::iterator it(m_variables.find("service.jid_escaping"));
		if (it != m_variables.end()) {
			it->second = ConfigValue(false);
		}
	}
}

Config *Config::createFromArgs(int argc, char **argv, std::string &error, std::string &host, int &port) {
	CommandLine cmdline;
	cmdline.addOption("help", 0, false, "help");
	cmdline.addOption("host", 'h', true, "Host to connect to", "");
	cmdline.addOption("port", 'p', true, "Port to connect to", "10000");
	cmdline.addOption("no-daemonize", 'n', false, "Do not run spectrum as daemon");
	cmdline.addOption("no-debug", 'd', false, "Create coredumps on crash");
	cmdline.addOption("jid", 'j', true, "Specify JID of transport manually", "");
	cmdline.addOption("config", 0, true, "Config file", "");
	cmdline.setPositional("config");
	std::string usage = cmdline.usage("Usage: spectrum <config_file.cfg>\nAllowed options");

	if (!cmdline.parse(argc, argv, error)) {
		error += "\n" + usage;
		return NULL;
	}

	if (cmdline.has("help")) {
		error = usage;
		return NULL;
	}

	host = cmdline.get("host");
	std::string portStr = cmdline.get("port");
	size_t pos = 0;
	try {
		port = std::stoi(portStr, &pos);
	}
	catch (...) {
		pos = 0;
	}
	if (portStr.empty() || pos != portStr.size()) {
		error = "the argument ('" + portStr + "') for option '--port' is invalid\n" + usage;
		return NULL;
	}

	std::string configFile = cmdline.get("config");
	Config *config = new Config(argc, argv);
	if (!config->load(configFile)) {
		error = "Can't open " + configFile + " configuration file.\n";
		delete config;
		return NULL;
	}
	return config;
}

}
