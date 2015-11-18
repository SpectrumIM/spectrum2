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

namespace Transport {

template <class myType>
const myType &safeAs(const boost::program_options::variable_value &var, const myType &def) {
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

/// Represents variable:value pairs.
typedef boost::program_options::variables_map Variables;

/// Represents config file.

/// It's used to load config file and allows others parts of libtransport to be configured
/// properly. Config files are text files which use "ini" format. Variables are divided into multiple
/// sections. Every class is configurable with some variables which change its behavior. Check particular
/// class documentation to get a list of all relevant variables for that class.
class Config {
	public:
		typedef std::map<std::string, boost::program_options::variable_value> SectionValuesCont;
		typedef std::map<std::string, boost::program_options::variable_value> UnregisteredCont;

		/// Constructor.
		Config(int argc = 0, char **argv = NULL) : m_argc(argc), m_argv(argv) {}

		/// Destructor
		virtual ~Config() {}

		/// Loads data from config file.
		
		/// You can pass your extra options which will be recognized by
		/// the parser using opts parameter.
		/// \param configfile path to config file
		/// \param opts extra options which will be recognized by a parser
		bool load(const std::string &configfile, boost::program_options::options_description &opts, const std::string &jid = "");

		bool load(std::istream &ifs, boost::program_options::options_description &opts, const std::string &jid = "");

		bool load(std::istream &ifs);

		/// Loads data from config file.
		
		/// This function loads only config variables needed by libtransport.
		/// \see load(const std::string &, boost::program_options::options_description &)
		/// \param configfile path to config file
		bool load(const std::string &configfile, const std::string &jid = "");

		bool reload();

		bool hasKey(const std::string &key) {
			return (m_variables.find(key) != m_variables.end() || m_unregistered.find(key) != m_unregistered.end()
					|| m_backendConfig.find(key) != m_backendConfig.end());
		}

		/// Returns value of variable defined by key.
		
		/// For variables in sections you can use "section.variable" key format.
		/// \param key config variable name
		const boost::program_options::variable_value &operator[] (const std::string &key) {
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
		boost::signal<void ()> onConfigReloaded;

		void updateBackendConfig(const std::string &backendConfig);
		boost::signal<void ()> onBackendConfigUpdated;

		static Config *createFromArgs(int argc, char **argv, std::string &error, std::string &host, int &port);
	
	private:
		int m_argc;
		char **m_argv;
		Variables m_variables;
		Variables m_backendConfig;
		std::map<std::string, boost::program_options::variable_value> m_unregistered;
		std::string m_file;
		std::string m_jid;
};

}
