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

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>

namespace Transport {

class Config;

namespace Util {

TRANSPORT_API void createDirectories(Transport::Config *config, const boost::filesystem::path& ph);

TRANSPORT_API void removeEverythingOlderThan(const std::vector<std::string> &dirs, time_t t);

TRANSPORT_API int getRandomPort(const std::string &s);

TRANSPORT_API std::string char2hex( char dec );
TRANSPORT_API std::string urlencode( const std::string &c );

TRANSPORT_API std::string base64Encode(const std::string &input);
TRANSPORT_API std::string base64Decode(const std::string &input);

#ifdef _WIN32
	TRANSPORT_API std::wstring utf8ToUtf16(const std::string& str);
#endif

}

}
