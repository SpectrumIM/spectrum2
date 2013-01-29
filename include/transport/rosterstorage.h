/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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
#include <algorithm>
#include <map>

#include "Swiften/Network/Timer.h"

namespace Transport {

class User;
class StorageBackend;
class Buddy;

// Stores buddies into DB Backend.
class RosterStorage {
	public:
		RosterStorage(User *user, StorageBackend *storageBackend);
		virtual ~RosterStorage();

		// Add buddy to store queue and store it in future. Nothing
		// will happen if buddy is already added.
		void storeBuddy(Buddy *buddy);

		void removeBuddy(Buddy *buddy);

		// Store all buddies from queue immediately. Returns true
		// if some buddies were stored.
		bool storeBuddies();

		// Remove buddy from storage queue.
		void removeBuddyFromQueue(Buddy *buddy);

	private:
		User *m_user;
		StorageBackend *m_storageBackend;
		std::map<std::string, Buddy *> m_buddies;
		Swift::Timer::ref m_storageTimer;
};

}
