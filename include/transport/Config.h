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

#include "transport/Event.h"

#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace Transport {

/// Value of one config variable.

/// Holds a string, int, bool, list (unregistered variable set more than once)
/// or vector (registered multi-value variable). as<T>() throws
/// std::bad_variant_access when the value has a different type.
class ConfigValue {
	public:
		ConfigValue() {}
		ConfigValue(const std::string &value) : m_value(value) {}
		ConfigValue(const char *value) : m_value(std::string(value)) {}
		ConfigValue(int value) : m_value(value) {}
		ConfigValue(bool value) : m_value(value) {}
		ConfigValue(const std::list<std::string> &value) : m_value(value) {}
		ConfigValue(const std::vector<std::string> &value) : m_value(value) {}

		bool empty() const { return m_value.index() == 0; }

		template <class T>
		const T &as() const { return std::get<T>(m_value); }

	private:
		std::variant<std::monostate, std::string, int, bool, std::list<std::string>, std::vector<std::string> > m_value;
};

template <class myType>
const myType &safeAs(const ConfigValue &var, const myType &def) {
	try  {
		return var.as<myType>();
	}
	catch(...) {
		return def;
	}
}

}

#define CONFIG_HAS_KEY(PTR, KEY) (*PTR).hasKey(KEY)
#define CONFIG_STRING(PTR, KEY) (*PTR)[KEY].as<std::string>()
#define CONFIG_INT(PTR, KEY) (*PTR)[KEY].as<int>()
#define CONFIG_BOOL(PTR, KEY) (*PTR)[KEY].as<bool>()
#define CONFIG_LIST(PTR, KEY) (*PTR)[KEY].as<std::list<std::string> >()
#define CONFIG_VECTOR(PTR, KEY) ((*PTR).hasKey(KEY) ? (*PTR)[KEY].as<std::vector<std::string> >() : std::vector<std::string>())

#define CONFIG_STRING_DEFAULTED(PTR, KEY, DEF) ((*PTR).hasKey(KEY) ? Transport::safeAs<std::string>((*PTR)[KEY], DEF) : DEF)
#define CONFIG_BOOL_DEFAULTED(PTR, KEY, DEF) ((*PTR).hasKey(KEY) ? Transport::safeAs<bool>((*PTR)[KEY], DEF) : DEF)
#define CONFIG_LIST_DEFAULTED(PTR, KEY, DEF) ((*PTR).hasKey(KEY) ? Transport::safeAs<std::list<std::string> >((*PTR)[KEY], DEF) : DEF)
#define CONFIG_INT_DEFAULTED(PTR, KEY, DEF) ((*PTR).hasKey(KEY) ? Transport::safeAs<int>((*PTR)[KEY], DEF) : DEF)

namespace Transport {

/// Represents config file.

/// It's used to load config file and allows others parts of libtransport to be configured
/// properly. Config files are text files which use "ini" format. Variables are divided into multiple
/// sections. Every class is configurable with some variables which change its behavior. Check particular
/// class documentation to get a list of all relevant variables for that class.
class TRANSPORT_API Config {
	public:
		typedef std::map<std::string, ConfigValue> SectionValuesCont;
		typedef std::map<std::string, ConfigValue> UnregisteredCont;

		/// Constructor.
		Config(int argc = 0, char **argv = NULL) : m_argc(argc), m_argv(argv) {}

		/// Destructor
		virtual ~Config() {}

		/// Loads data from config file.

		/// \param configfile path to config file
		/// \param jid overrides service.jid from the config file when not empty
		bool load(const std::string &configfile, const std::string &jid = "");

		bool load(std::istream &ifs, const std::string &jid = "");

		bool reload();

		bool hasKey(const std::string &key) {
			return (m_variables.find(key) != m_variables.end() || m_unregistered.find(key) != m_unregistered.end()
					|| m_backendConfig.find(key) != m_backendConfig.end());
		}

		/// Returns value of variable defined by key.

		/// For variables in sections you can use "section.variable" key format.
		/// \param key config variable name
		const ConfigValue &operator[] (const std::string &key) {
			if (m_variables.find(key) != m_variables.end()) {
				return m_variables[key];
			}
			if (m_backendConfig.find(key) != m_backendConfig.end()) {
				return m_backendConfig[key];
			}
			return m_unregistered[key];
		}

		SectionValuesCont getSectionValues(const std::string& sectionName);

		std::string getCommandLineArgs() const;

		/// Returns path to config file from which data were loaded.
		const std::string &getConfigFile() { return m_file; }

		/// This signal is emitted when config is loaded/reloaded.
		Event<void ()> onConfigReloaded;

		void updateBackendConfig(const std::string &backendConfig);
		Event<void ()> onBackendConfigUpdated;

		static Config *createFromArgs(int argc, char **argv, std::string &error, std::string &host, int &port);

	private:
		int m_argc;
		char **m_argv;
		std::string m_file;
		std::string m_jid;
		// Variables which got a value from the command line or a config
		// file; later loads don't override them, only their defaults.
		std::set<std::string> m_final;
		std::set<std::string> m_backendFinal;

	protected:
		std::map<std::string, ConfigValue> m_variables;
		std::map<std::string, ConfigValue> m_backendConfig;
		std::map<std::string, ConfigValue> m_unregistered;

};

}
