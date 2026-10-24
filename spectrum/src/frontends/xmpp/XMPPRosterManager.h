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

#include "transport/RosterManager.h"

#include <string>
#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Queries/GenericRequest.h"
#include "Swiften/Roster/SetRosterRequest.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Network/Timer.h"

namespace Transport {

class Buddy;
class User;
class Component;
class StorageBackend;
class RosterStorage;
class XMPPFrontend;

// TODO: Once Swiften GetRosterRequest will support setting to="", this can be removed
class AddressedRosterRequest : public Swift::GenericRequest<Swift::RosterPayload> {
	public:
		typedef std::shared_ptr<AddressedRosterRequest> ref;

		AddressedRosterRequest(Swift::IQRouter* router, Swift::JID to) :
				Swift::GenericRequest<Swift::RosterPayload>(Swift::IQ::Get, to, std::shared_ptr<Swift::Payload>(new Swift::RosterPayload()), router) {
		}
};

/// Manages roster of one XMPP user.
class XMPPRosterManager : public RosterManager {
	public:
		/// Creates new XMPPRosterManager.
		/// \param user User associated with this XMPPRosterManager.
		/// \param component Transport instance associated with this roster.
		XMPPRosterManager(User *user, Component *component);

		/// Destructor.
		virtual ~XMPPRosterManager();

		virtual void doRemoveBuddy(Buddy *buddy);
		virtual void doAddBuddy(Buddy *buddy);
		virtual void doUpdateBuddy(Buddy *buddy);
		virtual void doSendBuddyStatus(Buddy *buddy);
		virtual void doRequestAuthorization(const std::string &buddyName);
		virtual bool isValidBuddyName(const std::string &name, BuddyFlag flags);

		bool isRemoteRosterSupported() {
			return m_supportRemoteRoster;
		}

		/// Generates whole Presence stanzas with current status/show for this buddy.

		/// Presence stanzas are addressed to the user's bare JID.
		/// \return Presence stanzas: the buddy's own and the raw ones from the backend.
		std::vector<Swift::Presence::ref> &generatePresenceStanzas(Buddy *buddy);

		/// Remembers and forwards a raw presence of the buddy from the backend (features.rawxml).
		void handleRawPresence(Buddy *buddy, Swift::Presence::ref presence);

		Swift::RosterPayload::ref generateRosterPayload();

		void handleSubscription(Swift::Presence::ref presence);

		void sendBuddyRosterPush(Buddy *buddy);

		void sendBuddyRosterRemove(Buddy *buddy);

		void sendBuddySubscribePresence(Buddy *buddy);

		void sendBuddyUnsubscribePresence(Buddy *buddy);

		void sendBuddyPresences(Buddy *buddy, const Swift::JID &to);

		void sendCurrentPresences(const Swift::JID &to);

		void sendCurrentPresence(const Swift::JID &from, const Swift::JID &to);

		void sendUnavailablePresences(const Swift::JID &to);

	private:
		XMPPFrontend *getFrontend();
		void sendRIE();
		void handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, Swift::SetRosterRequest::ref request, const std::string &key);
		void handleRemoteRosterResponse(std::shared_ptr<Swift::RosterPayload> roster, Swift::ErrorPayload::ref error);

		Component *m_component;
		User *m_user;
		// Bare JID of the user (the XMPPUser is not constructed yet when we are).
		Swift::JID m_userJID;
		Swift::Timer::ref m_RIETimer;
		std::list <Swift::SetRosterRequest::ref> m_requests;
		bool m_supportRemoteRoster;
		AddressedRosterRequest::ref m_remoteRosterRequest;
		// Presences of each buddy, by lowercased buddy name.
		std::map<std::string, std::vector<Swift::Presence::ref> > m_presences;
};

}
