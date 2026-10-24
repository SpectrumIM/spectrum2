/**
 * libtransport -- C++ library for easy XMPP Transports development
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

#include "transport/CommandLine.h"

#include <sstream>

namespace Transport {

void CommandLine::addOption(const std::string &name, char shortName, bool takesValue, const std::string &description, const std::string &defaultValue) {
	m_options.push_back({name, shortName, takesValue, description, defaultValue});
}

void CommandLine::setPositional(const std::string &name) {
	m_positional = name;
}

const CommandLine::Option *CommandLine::findOption(const std::string &name) const {
	for (const Option &option : m_options) {
		if (option.name == name) {
			return &option;
		}
	}
	return NULL;
}

const CommandLine::Option *CommandLine::findOption(char shortName) const {
	for (const Option &option : m_options) {
		if (option.shortName != 0 && option.shortName == shortName) {
			return &option;
		}
	}
	return NULL;
}

bool CommandLine::parse(int argc, char **argv, std::string &error) {
	for (int i = 1; i < argc; i++) {
		std::string arg(argv[i]);
		const Option *option = NULL;
		std::string value;
		bool hasValue = false;

		if (arg.size() > 2 && arg.compare(0, 2, "--") == 0) {
			std::string name = arg.substr(2);
			size_t eq = name.find('=');
			if (eq != std::string::npos) {
				value = name.substr(eq + 1);
				name = name.substr(0, eq);
				hasValue = true;
			}
			option = findOption(name);
		}
		else if (arg.size() > 1 && arg[0] == '-' && arg[1] != '-') {
			option = findOption(arg[1]);
			if (option && option->takesValue && arg.size() > 2) {
				value = arg.substr(2);
				hasValue = true;
			}
		}
		else if (!m_positional.empty()) {
			if (m_values.find(m_positional) != m_values.end()) {
				error = "option '--" + m_positional + "' cannot be specified more than once";
				return false;
			}
			m_values[m_positional] = arg;
			continue;
		}

		// Unknown options are config variables, Config parses them.
		if (!option) {
			continue;
		}

		if (option->takesValue && !hasValue) {
			if (i + 1 >= argc) {
				error = "the required argument for option '--" + option->name + "' is missing";
				return false;
			}
			value = argv[++i];
		}
		m_values[option->name] = value;
	}
	return true;
}

bool CommandLine::has(const std::string &name) const {
	return m_values.find(name) != m_values.end();
}

std::string CommandLine::get(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator it = m_values.find(name);
	if (it != m_values.end()) {
		return it->second;
	}
	const Option *option = findOption(name);
	return option ? option->defaultValue : "";
}

std::string CommandLine::usage(const std::string &header) const {
	std::ostringstream os;
	os << header << ":\n";
	for (const Option &option : m_options) {
		std::string names = option.shortName ? std::string("-") + option.shortName + " [ --" + option.name + " ]" : "--" + option.name;
		if (option.takesValue) {
			names += " arg";
			if (!option.defaultValue.empty()) {
				names += " (=" + option.defaultValue + ")";
			}
		}
		os << "  " << names;
		if (names.size() < 38) {
			os << std::string(38 - names.size(), ' ');
		}
		else {
			os << "\n" << std::string(40, ' ');
		}
		os << option.description << "\n";
	}
	return os.str();
}

}
