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

namespace Transport {

class RosterManager;

typedef enum { 	BUDDY_NO_FLAG = 0,
				BUDDY_JID_ESCAPING = 2,
				BUDDY_IGNORE = 4
			} BuddyFlag;

/// Represents one legacy network Buddy.
class Buddy {
	public:
		/// Constructor.
		Buddy(RosterManager *rosterManager, long id = -1);

		/// Destructor
		virtual ~Buddy();
		
		/// Sets unique ID used to identify this buddy by StorageBackend. This is set
		/// by RosterStorage class once the buddy is stored into database or when the
		/// buddy is loaded from database.
		/// You should not need to set this ID manually.
		/// \param id ID
		void setID(long id);

		/// Returns unique ID used to identify this buddy by StorageBackend.
		/// \see Buddy::setID(long)
		/// \return ID
		long getID();

		/// Returns full JID of this buddy.
		/// \param hostname hostname used as domain in returned JID
		/// \return full JID of this buddy
		const Swift::JID &getJID();

		/// Generates whole Presennce stanza with current status/show for this buddy.
		/// Presence stanza does not containt "to" attribute, it has to be added manually.
		/// \param features features used in returned stanza
		/// \param only_new if True, this function returns Presence stanza only if it's different
		/// than the previously generated one.
		/// \return Presence stanza or NULL.
		Swift::Presence::ref generatePresenceStanza(int features, bool only_new = false);

		/// Marks this buddy as available.
		void setOnline();

		/// Marks this buddy as offline.
		void setOffline();

		/// Returns true if this buddy is marked as available/online.
		/// \return true if this buddy is marked as available/online.
		bool isOnline();

		/// Sets current subscription.
		/// \param subscription "to", "from", "both", "ask"
		void setSubscription(const std::string &subscription);

		/// Returns current subscription
		/// \return subscription "to", "from", "both", "ask"
		const std::string &getSubscription();

		/// Sets this buddy's flags.
		/// \param flags flags
		void setFlags(BuddyFlag flags);

		/// Returns this buddy's flags.
		/// \param flags flags
		BuddyFlag getFlags();

		/// Returns RosterManager associated with this buddy
		/// \return rosterManager
		RosterManager *getRosterManager() { return m_rosterManager; }

		/// Returns legacy network username which does not contain unsafe characters,
		/// so it can be used in JIDs.
		std::string getSafeName();

		void buddyChanged();

		void handleVCardReceived(const std::string &id, const Swift::JID &to, Swift::VCard::ref vcard);

		boost::signal<void ()> onBuddyChanged;

		virtual void getVCard(const std::string &id, const Swift::JID &to) = 0;

		/// Returns legacy network username of this buddy. (for example UIN for ICQ,
		/// JID for Jabber, ...).
		/// \return legacy network username
		virtual std::string getName() = 0;

		/// Returns alias (nickname) of this buddy.
		/// \return alias (nickname)
		virtual std::string getAlias() = 0;

		/// Returns list of groups this buddy is in.
		/// \return groups
		virtual std::vector<std::string> getGroups() = 0;

		/// Returns current legacy network status and statuMessage of this buddy.
		/// \param status current status/show is stored here
		/// \param statusMessage current status message is stored here
		/// \return true if status was stored successfully
		virtual bool getStatus(Swift::StatusShow &status, std::string &statusMessage) = 0;

		/// Returns SHA-1 hash of buddy icon (avatar) or empty string if there is no avatar
		/// for this buddy.
		/// \return avatar hash or empty string.
		virtual std::string getIconHash() = 0;

		static std::string JIDToLegacyName(const Swift::JID &jid);

	private:
		void generateJID();

		long m_id;
		bool m_online;
		std::string m_subscription;
		Swift::Presence::ref m_lastPresence;
		Swift::JID m_jid;
		BuddyFlag m_flags;
		RosterManager *m_rosterManager;
};

}
