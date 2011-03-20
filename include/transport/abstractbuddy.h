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
#include "transport/transport.h"

#include "Swiften/Swiften.h"

typedef enum { 	SPECTRUM_BUDDY_NO_FLAG = 0,
				SPECTRUM_BUDDY_JID_ESCAPING = 2,
				SPECTRUM_BUDDY_IGNORE = 4
			} SpectrumBuddyFlag;

namespace Transport {

class RosterManager;

// Wrapper for PurpleBuddy.
class AbstractBuddy {
	public:
		AbstractBuddy(long id);
		virtual ~AbstractBuddy();
		
		// Sets/gets ID used to identify this buddy for example by storage backend.
		void setId(long id);
		long getId();

		// Returns full JID.
		Swift::JID getJID(const std::string &hostname);

		// Generates whole <presence> stanza without "to" attribute. That attribute
		// has to be added manually.
		// only_new - if the stanza is the same as previous generated one, returns NULL.
		Swift::Presence::ref generatePresenceStanza(int features, bool only_new = false);

		// Sets online/offline state information.
		void setOnline();
		void setOffline();

		// Returns true if online.
		bool isOnline();

		// Sets/gets current subscription.
		// TODO: rewrite me to use SpectrumSubscriptionType!
		void setSubscription(const std::string &subscription);
		const std::string &getSubscription();

		// Sets SpectrumBuddyFlags.
		void setFlags(int flags);

		// Returns flags.
		int getFlags();

		void setRosterManager(RosterManager *rosterManager) { m_rosterManager = rosterManager; }
		RosterManager *getRosterManager() { return m_rosterManager; }

		// Returns buddy's name (so for example UIN for ICQ, JID for XMPP...).
		virtual std::string getName() = 0;

		// Returns buddy's alias (nickname).
		virtual std::string getAlias() = 0;

		// Returns buddy's group name.
		virtual std::vector<std::string> getGroups() = 0;

		// Returns name which doesn't contain unsafe characters, so it can be used.
		// in JIDs.
		virtual std::string getSafeName() = 0;

		// Stores current status in `status` and current status message in `statusMessage`.
		// Returns true if data can be stored.
		virtual bool getStatus(Swift::StatusShow &status, std::string &statusMessage) = 0;

		// Returns SHA-1 hash of buddy icon (avatar) or empty string if there is no avatar.
		virtual std::string getIconHash() = 0;

	private:
		long m_id;
		bool m_online;
		std::string m_subscription;
		Swift::Presence::ref m_lastPresence;
		int m_flags;
		RosterManager *m_rosterManager;
};

}
