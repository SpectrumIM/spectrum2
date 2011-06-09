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

#include "transport/rosterresponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "Swiften/Swiften.h"
#include "transport/user.h"
#include "transport/usermanager.h"
#include "transport/rostermanager.h"
#include "transport/buddy.h"

using namespace Swift;
using namespace boost;

namespace Transport {

RosterResponder::RosterResponder(Swift::IQRouter *router, UserManager *userManager) : Swift::Responder<RosterPayload>(router) {
	m_userManager = userManager;
}

RosterResponder::~RosterResponder() {
}

bool RosterResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::RosterPayload> payload) {
	// Get means we're in server mode and user wants to fetch his roster.
	// For now we send empty reponse, but TODO: Get buddies from database and send proper stored roster.
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		// Client can send jabber:iq:roster IQ before presence, so we do little hack here to
		// trigger logging in.
		// UserManager should create user now, if everything is OK.
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(to);
		response->setFrom(from);
		response->setType(Swift::Presence::Available);
		m_userManager->handlePresence(response);

		// if it's not created, lets finish this get
		user = m_userManager->getUser(from.toBare().toString());
		if (!user) {
			sendResponse(from, id, boost::shared_ptr<RosterPayload>(new RosterPayload()));
			return true;
		}
	}
	sendResponse(from, id, user->getRosterManager()->generateRosterPayload());
	return true;
}

bool RosterResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, boost::shared_ptr<Swift::RosterPayload> payload) {
	sendResponse(from, id, boost::shared_ptr<RosterPayload>(new RosterPayload()));

	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		return true;
	}

	if (payload->getItems().size() == 0) {
		return true;
	}

	Swift::RosterItemPayload item = payload->getItems()[0];

	Buddy *buddy = user->getRosterManager()->getBuddy(Buddy::JIDToLegacyName(item.getJID()));
	if (buddy) {
		if (item.getSubscription() == Swift::RosterItemPayload::Remove) {
			std::cout << "BUDDY REMOVED\n";
		}
		else {
			std::cout << "BUDDY UPDATED\n";
		}
	}
	else if (item.getSubscription() != Swift::RosterItemPayload::Remove) {
		// Roster push for this new buddy is sent by RosterManager
		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = item.getName();
		buddyInfo.legacyName = Buddy::JIDToLegacyName(item.getJID());
		buddyInfo.subscription = "both";
		buddyInfo.flags = 0;

		buddy = user->getComponent()->getFactory()->createBuddy(user->getRosterManager(), buddyInfo);
		user->getRosterManager()->setBuddy(buddy);
		std::cout << "BUDDY ADDED\n";
	}

	return true;
}

}
