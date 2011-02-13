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

#include <string>
#include <map>
#include "Swiften/Swiften.h"
#include "transport/storagebackend.h"
#include "transport/config.h"
#include <sqlite3.h>

namespace Transport {

class SQLite3Backend : public StorageBackend
{
	public:
		SQLite3Backend(Config::Variables &config);
		~SQLite3Backend();

		bool connect();
		bool createDatabase();

		void setUser(const UserInfo &user);
		bool getUser(const std::string &barejid, UserInfo &user);
		void setUserOnline(long id, bool online);
		void removeUser(long id);

		bool getBuddies(long id, std::list<std::string> &roster);

	private:
		bool exec(const std::string &query);

		sqlite3 *m_db;
		Config::Variables m_config;
		std::string m_prefix;
};

}
