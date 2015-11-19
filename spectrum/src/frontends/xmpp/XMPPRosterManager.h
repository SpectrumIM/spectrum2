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
#include <map>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
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

// TODO: Once Swiften GetRosterRequest will support setting to="", this can be removed
class AddressedRosterRequest : public Swift::GenericRequest<Swift::RosterPayload> {
	public:
		typedef boost::shared_ptr<AddressedRosterRequest> ref;

		AddressedRosterRequest(Swift::IQRouter* router, Swift::JID to) :
				Swift::GenericRequest<Swift::RosterPayload>(Swift::IQ::Get, to, boost::shared_ptr<Swift::Payload>(new Swift::RosterPayload()), router) {
		}
};

/// Manages roster of one XMPP user.
class XMPPRosterManager : public RosterManager {
	public:
		typedef std::map<std::string, Buddy *, std::less<std::string>, boost::pool_allocator< std::pair<std::string, Buddy *> > > BuddiesMap;
		/// Creates new XMPPRosterManager.
		/// \param user User associated with this XMPPRosterManager.
		/// \param component Transport instance associated with this roster.
		XMPPRosterManager(User *user, Component *component);

		/// Destructor.
		virtual ~XMPPRosterManager();

		virtual void doRemoveBuddy(Buddy *buddy);
		virtual void doAddBuddy(Buddy *buddy);
		virtual void doUpdateBuddy(Buddy *buddy);



		bool isRemoteRosterSupported() {
			return m_supportRemoteRoster;
		}

		boost::signal<void (Buddy *buddy)> onBuddyAdded;
		
		boost::signal<void (Buddy *buddy)> onBuddyRemoved;

		void handleBuddyChanged(Buddy *buddy);

		void sendBuddyRosterPush(Buddy *buddy);

		void sendBuddyRosterRemove(Buddy *buddy);

	private:
		void sendRIE();
		void handleBuddyRosterPushResponse(Swift::ErrorPayload::ref error, Swift::SetRosterRequest::ref request, const std::string &key);
		void handleRemoteRosterResponse(boost::shared_ptr<Swift::RosterPayload> roster, Swift::ErrorPayload::ref error);

		Component *m_component;
		RosterStorage *m_rosterStorage;
		User *m_user;
		Swift::Timer::ref m_setBuddyTimer;
		Swift::Timer::ref m_RIETimer;
		std::list <Swift::SetRosterRequest::ref> m_requests;
		bool m_supportRemoteRoster;
		AddressedRosterRequest::ref m_remoteRosterRequest;
};

}
