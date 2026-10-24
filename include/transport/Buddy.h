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
#include <vector>
#include <algorithm>
#include "transport/protocol.pb.h"

namespace Transport {

class RosterManager;
class User;

typedef enum { 	BUDDY_NO_FLAG = 0,
				BUDDY_JID_ESCAPING = 2,
				BUDDY_IGNORE = 4,
				BUDDY_BLOCKED = 8,
			} BuddyFlag;

/// Represents one legacy network Buddy.
class Buddy {
	public:
		typedef enum { 	Ask,
						Both,
					} Subscription;
		/// Constructor.

		/// \param rosterManager RosterManager associated with this buddy.
		/// \param id ID which identifies the buddy in database or -1 if it's new buddy which is
		/// not in database yet.
		Buddy(RosterManager *rosterManager, long id = -1, BuddyFlag flags = BUDDY_NO_FLAG);

		/// Destructor
		virtual ~Buddy();

		/// Sets unique ID used to identify this buddy by StorageBackend.

		/// This is set
		/// by RosterStorage class once the buddy is stored into database or when the
		/// buddy is loaded from database.
		/// You should not need to set this ID manually.
		/// \param id ID
		void setID(long id);

		/// Returns unique ID used to identify this buddy by StorageBackend.

		/// \return ID which identifies the buddy in database or -1 if it's new buddy which is
		/// not in database yet.
		long getID();

		void setBlocked(bool block) {
			if (block)
				m_flags = (BuddyFlag) (m_flags | BUDDY_BLOCKED);
			else
				m_flags = (BuddyFlag) (m_flags & ~BUDDY_BLOCKED);
		}

		bool isBlocked() {
			return (m_flags & BUDDY_BLOCKED)  != 0;
		}

		/// Sets current subscription.

		/// \param subscription "to", "from", "both", "ask"
		void setSubscription(Subscription subscription);

		/// Returns current subscription

		/// \return subscription "to", "from", "both", "ask"
		Subscription getSubscription();

		/// Sets this buddy's flags.

		/// \param flags flags
		void setFlags(BuddyFlag flags);

		/// Returns this buddy's flags.

		/// \param flags flags
		BuddyFlag getFlags();

		/// Returns RosterManager associated with this buddy.

		/// \return RosterManager associated with this buddy.
		RosterManager *getRosterManager() { return m_rosterManager; }

		/// Sends this buddy's current status to the user.
		void sendPresence();

		/// Returns legacy network username of this buddy. (for example UIN for ICQ, JID for Jabber, ...).

		/// \return legacy network username
		virtual std::string getName() = 0;

		/// Returns alias (nickname) of this buddy.

		/// \return alias (nickname)
		virtual std::string getAlias() = 0;

		/// Returns list of groups this buddy is in.

		/// \return groups
		virtual std::vector<std::string> getGroups() = 0;

		/// Returns current legacy network status and statuMessage of this buddy.

		/// \param status current status is stored here
		/// \param statusMessage current status message is stored here
		/// \return true if status was stored successfully
		virtual bool getStatus(pbnetwork::StatusType &status, std::string &statusMessage) = 0;

		/// Returns SHA-1 hash of buddy icon (avatar) or empty string if there is no avatar for this buddy.

		/// \return avatar hash or empty string.
		virtual std::string getIconHash() = 0;

		virtual bool isAvailable() = 0;

	private:
		long m_id;
		BuddyFlag m_flags;
		RosterManager *m_rosterManager;
		Subscription m_subscription;
};

}
