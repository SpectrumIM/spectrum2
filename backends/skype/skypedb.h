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

#include "glib.h"
#include <dbus-1.0/dbus/dbus-glib-lowlevel.h>
#include "sqlite3.h"
#include <iostream>
#include <map>

class SkypePlugin;

namespace SkypeDB {
	bool getAvatar(const std::string &db, const std::string &name, std::string &avatar);
	bool loadBuddies(SkypePlugin *np, const std::string &db, std::string &user, std::map<std::string, std::string> &group_map);
}

