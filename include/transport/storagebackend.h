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

/// Represents all data needed to be stored in database.
struct UserInfo {
	long id;				///< id of user used as primary key in database
	std::string jid;		///< barejid of XMPP user
	std::string uin;		///< legacy network username
	std::string password;	///< password for legacy network
	std::string language;	///< user's preferred language
	std::string encoding;	///< user's preferred encoding
	bool vip;				///< true if user is VIP
};

/// Abstract class defining storage backends.
class StorageBackend
{
	public:
		/// Virtual desctructor.
		virtual ~StorageBackend() {}

		/// connect
		virtual bool connect() = 0;

		/// createDatabase
		virtual bool createDatabase() = 0;

		/// setUser
		virtual void setUser(const UserInfo &user) = 0;

		/// getuser
		virtual bool getUser(const std::string &barejid, UserInfo &user) = 0;

		/// setUserOnline
		virtual void setUserOnline(long id, bool online) = 0;

		/// removeUser
		virtual bool removeUser(long id) = 0;

		/// getBuddies
		virtual bool getBuddies(long id, std::list<std::string> &roster) = 0;

		/// onStorageError
		boost::signal<void (const std::string &statement, const std::string &error)> onStorageError;

};

}
