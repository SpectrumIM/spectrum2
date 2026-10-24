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

#include "RosterResponder.h"

#include <iostream>
#include <boost/bind.hpp>
#include "Swiften/Queries/IQRouter.h"
#include "transport/User.h"
#include "transport/UserManager.h"
#include "transport/RosterManager.h"
#include "transport/Buddy.h"
#include "transport/Logging.h"
#include "transport/Factory.h"
#include "transport/Transport.h"
#include "XMPPFrontend.h"
#include "XMPPRosterManager.h"
#include "jidutils.h"
#include "transport/Frontend.h"

using namespace Swift;

namespace Transport {

DEFINE_LOGGER(rosterResponderLogger, "RosterResponder");

RosterResponder::RosterResponder(Swift::IQRouter *router, UserManager *userManager) : Swift::Responder<RosterPayload>(router) {
	m_userManager = userManager;
	m_router = router;
}

RosterResponder::~RosterResponder() {
}

bool RosterResponder::handleGetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, std::shared_ptr<Swift::RosterPayload> payload) {
	// Get means we're in server mode and user wants to fetch his roster.
	// For now we send empty reponse, but TODO: Get buddies from database and send proper stored roster.
	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		sendResponse(from, id, std::shared_ptr<RosterPayload>(new RosterPayload()));
		LOG4CXX_WARN(rosterResponderLogger, from.toBare().toString() << ": User is not logged in");
		return true;
	}
	XMPPRosterManager *rosterManager = static_cast<XMPPRosterManager *>(user->getRosterManager());
	sendResponse(from, id, rosterManager->generateRosterPayload());
	rosterManager->sendCurrentPresences(from);
	return true;
}

// The roster item as the core sees it.
static pbnetwork::Buddy toBuddy(User *user, Buddy *buddy, const Swift::RosterItemPayload &item) {
	pbnetwork::Buddy b;
	b.set_username(user->getUserName());
	b.set_buddyname(buddy->getName());
	b.set_alias(item.getName());
	for (const std::string &group : item.getGroups()) {
		b.add_group(group);
	}
	return b;
}

bool RosterResponder::handleSetRequest(const Swift::JID& from, const Swift::JID& to, const std::string& id, std::shared_ptr<Swift::RosterPayload> payload) {
	sendResponse(from, id, std::shared_ptr<RosterPayload>(new RosterPayload()));

	User *user = m_userManager->getUser(from.toBare().toString());
	if (!user) {
		LOG4CXX_WARN(rosterResponderLogger, from.toBare().toString() << ": User is not logged in");
		return true;
	}

	if (payload->getItems().size() == 0) {
		LOG4CXX_WARN(rosterResponderLogger, from.toBare().toString() << ": Roster push with no item");
		return true;
	}

	Swift::RosterItemPayload item = payload->getItems()[0];

	if (item.getJID().getNode().empty()) {
		return true;
	}

	Buddy *buddy = JIDUtils::JIDToBuddy(item.getJID(), user);
	if (buddy) {
		if (item.getSubscription() == Swift::RosterItemPayload::Remove) {
			LOG4CXX_INFO(rosterResponderLogger, from.toBare().toString() << ": Removing buddy " << buddy->getName());
			user->getComponent()->getFrontend()->onBuddyRemoved(buddy);

			// send roster push here
			Swift::SetRosterRequest::ref request = Swift::SetRosterRequest::create(payload, Swift::JID(user->getUserName()).toBare(), m_router);
			request->send();
		}
		else {
			LOG4CXX_INFO(rosterResponderLogger, from.toBare().toString() << ": Updating buddy " << buddy->getName());
			user->getComponent()->getFrontend()->onBuddyUpdated(buddy, toBuddy(user, buddy, item));
		}
	}
	else if (item.getSubscription() != Swift::RosterItemPayload::Remove) {
		// Roster push for this new buddy is sent by RosterManager
		BuddyInfo buddyInfo;
		buddyInfo.id = -1;
		buddyInfo.alias = item.getName();
		buddyInfo.legacyName = JIDUtils::JIDToLegacyName(item.getJID(), user);
		buddyInfo.subscription = "both";
		buddyInfo.flags = JIDUtils::buddyFlagsFromJID(item.getJID());
		LOG4CXX_INFO(rosterResponderLogger, from.toBare().toString() << ": Adding buddy " << buddyInfo.legacyName);

		buddy = user->getComponent()->getFactory()->createBuddy(user->getRosterManager(), buddyInfo);
		user->getRosterManager()->setBuddy(buddy);
		user->getComponent()->getFrontend()->onBuddyAdded(buddy, toBuddy(user, buddy, item));
	}

	return true;
}

}
