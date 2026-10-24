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
#include <string_view>
#include <filesystem>

namespace Transport {

class Config;

namespace Util {

TRANSPORT_API void createDirectories(Transport::Config *config, const std::filesystem::path& ph);

/// Replaces every occurrence of \p from in \p str with \p to.
TRANSPORT_API void replaceAll(std::string &str, std::string_view from, std::string_view to);
TRANSPORT_API std::string replaceAllCopy(std::string str, std::string_view from, std::string_view to);

/// Splits \p str on every \p delim. Empty tokens are kept, so "" yields {""}
/// and "a\n" yields {"a", ""}.
TRANSPORT_API std::vector<std::string> split(std::string_view str, char delim);

/// ASCII case-insensitive prefix/suffix tests.
TRANSPORT_API bool iStartsWith(std::string_view str, std::string_view prefix);
TRANSPORT_API bool iEndsWith(std::string_view str, std::string_view suffix);

/// Lowercases UTF-8 text using simple (1:1) Unicode case mapping for the
/// Latin, Greek, Cyrillic, Armenian and Georgian scripts. Invalid UTF-8 is
/// lowercased as ASCII only.
TRANSPORT_API std::string toLowerUTF8(std::string_view str);

/// Returns a random (version 4) UUID in its canonical textual form.
TRANSPORT_API std::string generateUUID();

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
