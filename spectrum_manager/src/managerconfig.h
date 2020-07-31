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

#include <boost/signals2.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>

/// Represents variable:value pairs.
typedef boost::program_options::variables_map Variables;

/// Represents config file.

/// It's used to load config file and allows others parts of libtransport to be configured
/// properly. ManagerConfig files are text files which use "ini" format. Variables are divided into multiple
/// sections. Every class is configurable with some variables which change its behavior. Check particular
/// class documentation to get a list of all relevant variables for that class.
class ManagerConfig {
	public:
		/// Constructor.
		ManagerConfig() {}

		/// Destructor
		virtual ~ManagerConfig() {}

		/// Loads data from config file.
		
		/// You can pass your extra options which will be recognized by
		/// the parser using opts parameter.
		/// \param configfile path to config file
		/// \param opts extra options which will be recognized by a parser
		bool load(const std::string &configfile, boost::program_options::options_description &opts);

		/// Loads data from config file.
		
		/// This function loads only config variables needed by libtransport.
		/// \see load(const std::string &, boost::program_options::options_description &)
		/// \param configfile path to config file
		bool load(const std::string &configfile);

		/// Returns value of variable defined by key.
		
		/// For variables in sections you can use "section.variable" key format.
		/// \param key config variable name
		const boost::program_options::variable_value &operator[] (const std::string &key) {
			return m_variables[key];
		}

		bool hasKey(const std::string &key) {
			return m_variables.find(key) != m_variables.end();
		}

		/// Returns path to config file from which data were loaded.
		const std::string &getManagerConfigFile() { return m_file; }

		/// This signal is emitted when config is loaded/reloaded.
		boost::signals2::signal<void ()> onManagerConfigReloaded;
	
	private:
		Variables m_variables;
		std::string m_file;
};
