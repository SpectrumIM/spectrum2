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

namespace Transport {

struct UserInfo {
	long id;
	std::string jid;
	std::string uin;
	std::string password;
	std::string language;
	std::string encoding;
	bool vip;
};

class StorageBackend
{
	public:
		virtual ~StorageBackend() {}

		virtual bool connect() = 0;
		virtual bool createDatabase() = 0;

		virtual void setUser(const UserInfo &user) = 0;
		virtual bool getUser(const std::string &barejid, UserInfo &user) = 0;
		virtual void setUserOnline(long id, bool online) = 0;
		virtual void removeUser(long id) = 0;

		virtual bool getBuddies(long id, std::list<std::string> &roster) = 0;

		boost::signal<void (const std::string &statement, const std::string &error)> onStorageError;

};

}
