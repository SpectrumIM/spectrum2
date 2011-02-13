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

namespace Transport {

class User;
class Component;

// Class for managing online XMPP users.
class UserManager
{
	public:
		UserManager(Component *component);
		~UserManager();

		// User *
		User *getUserByJID(const std::string &barejid);

		// Returns count of online users;
		int userCount();

		void removeUser(User *user) {}

	private:
		void handlePresence(Swift::Presence::ref presence);

		long m_onlineBuddies;
		User *m_cachedUser;
		std::map<std::string, User *> m_users;
};

}
