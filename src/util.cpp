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

#include "transport/util.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/cast.hpp>

using namespace boost::filesystem;

using namespace boost;

namespace Transport {

namespace Util {

void removeEverythingOlderThan(const std::vector<std::string> &dirs, time_t t) {
	BOOST_FOREACH(const std::string &dir, dirs) {
		path p(dir);

		try {
			if (!exists(p)) {
				continue;
			}
			if (!is_directory(p)) {
				continue;
			}

			directory_iterator end_itr;
			for (directory_iterator itr(p); itr != end_itr; ++itr) {
				if (last_write_time(itr->path()) < t) {
					try {
						if (is_regular(itr->path())) {
							remove(itr->path());
						}
						else if (is_directory(itr->path())) {
							std::vector<std::string> nextDirs;
							nextDirs.push_back(itr->path().string());
							removeEverythingOlderThan(nextDirs, t);
							if (is_empty(itr->path())) {
								remove_all(itr->path());
							}
						}
					}
					catch (const filesystem_error& ex) {
						
					}
				}
			}


		}
		catch (const filesystem_error& ex) {
			
		}
	}
}

std::string encryptPassword(const std::string &password, const std::string &key) {
	std::string encrypted;
	encrypted.resize(password.size());
	for (int i = 0; i < password.size(); i++) {
		char c = password[i];
		char keychar = key[i % key.size()];
		c += keychar;
		encrypted[i] = c;
	}

	encrypted = Swift::Base64::encode(Swift::createByteArray(encrypted));
	return encrypted;
}

std::string decryptPassword(std::string &encrypted, const std::string &key) {
	encrypted = Swift::byteArrayToString(Swift::Base64::decode(encrypted));
	std::string password;
	password.resize(encrypted.size());
	for (int i = 0; i < encrypted.size(); i++) {
		char c = encrypted[i];
		char keychar = key[i % key.size()];
		c -= keychar;
		password[i] = c;
	}

	return password;
}

std::string serializeGroups(const std::vector<std::string> &groups) {
	std::string ret;
	BOOST_FOREACH(const std::string &group, groups) {
		ret += group + "\n";
	}
	if (!ret.empty()) {
		ret.erase(ret.end() - 1);
	}
	return ret;
}

std::vector<std::string> deserializeGroups(std::string &groups) {
	std::vector<std::string> ret;
	if (groups.empty()) {
		return ret;
	}

	boost::split(ret, groups, boost::is_any_of("\n"));
	if (ret.back().empty()) {
		ret.erase(ret.end() - 1);
	}
	return ret;
}

int getRandomPort(const std::string &s) {
	unsigned long r = 0;
	BOOST_FOREACH(char c, s) {
		r += (int) c;
	}
	srand(time(NULL) + r);
	return 30000 + rand() % 10000;
}

#ifdef _WIN32
std::wstring utf8ToUtf16(const std::string& str)
{
	try
	{
		if (str.empty())
			return L"";

		// First request the size of the required UTF-16 buffer
		int numRequiredBytes = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), boost::numeric_cast<int>(str.size()), NULL, 0);
		if (!numRequiredBytes)
			return L"";

		// Allocate memory for the UTF-16 string
		std::vector<wchar_t> utf16Str(numRequiredBytes);

		int numConverted = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), boost::numeric_cast<int>(str.size()), &utf16Str[0], numRequiredBytes);
		if (!numConverted)
			return L"";

		std::wstring wstr(&utf16Str[0], numConverted);
		return wstr;
	}
	catch (...)
	{
		// I don't believe libtransport is exception-safe so we'll just return an empty string instead
		return L"";
	}
}
#endif // _WIN32


}

}
