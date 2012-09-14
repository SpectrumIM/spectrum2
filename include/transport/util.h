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
#include "Swiften/StringCodecs/Base64.h"

namespace Transport {

namespace Util {

void removeEverythingOlderThan(const std::vector<std::string> &dirs, time_t t);

std::string encryptPassword(const std::string &password, const std::string &key);

std::string decryptPassword(std::string &encrypted, const std::string &key);

std::string serializeGroups(const std::vector<std::string> &groups);

std::vector<std::string> deserializeGroups(std::string &groups);

int getRandomPort(const std::string &s);

#ifdef _WIN32
	std::wstring utf8ToUtf16(const std::string& str);
#endif

}

}
