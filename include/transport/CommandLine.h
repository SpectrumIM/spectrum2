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

#pragma once

#include <map>
#include <string>
#include <vector>

namespace Transport {

/// Command line parser for the spectrum2 and backend executables.

/// Accepts "--name value", "--name=value", "-n value" and "-nvalue".
/// Unknown options are ignored: they are config variables
/// (e.g. --service.backend_id=1) which Config reads itself.
class TRANSPORT_API CommandLine {
	public:
		/// Adds an option. shortName is 0 when the option has no short form.
		void addOption(const std::string &name, char shortName, bool takesValue, const std::string &description, const std::string &defaultValue = "");

		/// Arguments which are not options are stored as value of this option.
		void setPositional(const std::string &name);

		/// Returns false and sets error on a missing option value or a second positional argument.
		bool parse(int argc, char **argv, std::string &error);

		/// True if the option was given on the command line.
		bool has(const std::string &name) const;

		/// Value given on the command line, or the option's default.
		std::string get(const std::string &name) const;

		/// Help text listing all options.
		std::string usage(const std::string &header) const;

	private:
		struct Option {
			std::string name;
			char shortName;
			bool takesValue;
			std::string description;
			std::string defaultValue;
		};

		const Option *findOption(const std::string &name) const;
		const Option *findOption(char shortName) const;

		std::vector<Option> m_options;
		std::string m_positional;
		std::map<std::string, std::string> m_values;
};

}
