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

#include "transport/Event.h"
#include "transport/Buddy.h"

#include <string>
#include <algorithm>
#include <map>
#include <functional>

namespace Transport {

class Buddy;
class User;
class Component;
class StorageBackend;
class RosterStorage;

/// Manages roster of one user.
class RosterManager {
	public:
		typedef std::map<std::string, Buddy *> BuddiesMap;
		/// Creates new RosterManager.
		/// \param user User associated with this RosterManager.
		/// \param component Transport instance associated with this roster.
		RosterManager(User *user, Component *component);

		/// Destructor.
		virtual ~RosterManager();

		virtual void doRemoveBuddy(Buddy *buddy) = 0;
		virtual void doAddBuddy(Buddy *buddy) = 0;
		virtual void doUpdateBuddy(Buddy *buddy) = 0;

		/// Sends the buddy's current status to the user.
		virtual void doSendBuddyStatus(Buddy *buddy) {}

		/// The buddy asks the user for authorization (backend TYPE_AUTH_REQUEST).
		virtual void doRequestAuthorization(const std::string &buddyName) {}

		/// Returns true if the frontend can address a buddy with this name and flags.
		virtual bool isValidBuddyName(const std::string &name, BuddyFlag flags) { return !name.empty(); }

		/// Associates the buddy with this roster,
		/// and if the buddy is not already in user's roster, the frontend adds it there
		/// (doAddBuddy).
		/// \param buddy Buddy
		void setBuddy(Buddy *buddy);

		/// Deassociates the buddy with this roster.
		/// \param buddy Buddy.
		void unsetBuddy(Buddy *buddy);

		/// Removes buddy from this roster, removes it on the frontend side and deletes it.
		/// \param name Buddy name.
		void removeBuddy(const std::string &name);

		Buddy *getBuddy(const std::string &name);

		void setStorageBackend(StorageBackend *storageBackend);

		void storeBuddy(Buddy *buddy);

		/// Returns user associated with this roster.
		/// \return User
		User *getUser() { return m_user; }

		const BuddiesMap &getBuddies() {
			return m_buddies;
		}

		/// Called when new Buddy is added to this roster.
		/// \param buddy newly added Buddy
		Event<void (Buddy *buddy)> onBuddySet;

		/// Called when Buddy has been removed from this roster.
		/// \param buddy removed Buddy
		Event<void (Buddy *buddy)> onBuddyUnset;

		/// The user added the buddy (forwarded to the backend).
		Event<void (Buddy *buddy)> onBuddyAdded;

		/// The user removed the buddy (forwarded to the backend).
		Event<void (Buddy *buddy)> onBuddyRemoved;

	protected:
		BuddiesMap m_buddies;

	private:
		Component *m_component;
		RosterStorage *m_rosterStorage;
		User *m_user;
};

}
