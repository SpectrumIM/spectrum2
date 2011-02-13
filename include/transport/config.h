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

#define CONFIG_STRING(PTR, KEY) (*PTR)[KEY].as<std::string>()
#define CONFIG_INT(PTR, KEY) (*PTR)[KEY].as<int>()
#define CONFIG_BOOL(PTR, KEY) (*PTR)[KEY].as<bool>()
#define CONFIG_LIST(PTR, KEY) (*PTR)[KEY].as<std::list<std::string> >()

namespace Transport {

typedef boost::program_options::variables_map Variables;

class Config {
	public:
		Config() {}
		virtual ~Config() {}
		bool load(const std::string &configfile, boost::program_options::options_description &opts);
		bool load(const std::string &configfile);

		const boost::program_options::variable_value &operator[] (const std::string &key) {
			return m_variables[key];
		}
	
	private:
		Variables m_variables;
};

}
