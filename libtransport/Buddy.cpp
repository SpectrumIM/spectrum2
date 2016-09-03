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

#include "transport/Buddy.h"
#include "transport/RosterManager.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/UserManager.h"
#include "transport/Frontend.h"

#include "Swiften/Elements/VCardUpdate.h"
#include "Swiften/Elements/Presence.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

namespace Transport {

Buddy::Buddy(RosterManager *rosterManager, long id, BuddyFlag flags) : m_id(id), m_flags(flags), m_rosterManager(rosterManager),
	m_subscription(Ask) {
}

Buddy::~Buddy() {
}

void Buddy::sendPresence() {
	std::vector<Swift::Presence::ref> &presences = generatePresenceStanzas(255);
	BOOST_FOREACH(Swift::Presence::ref presence, presences) {
		m_rosterManager->getUser()->getComponent()->getFrontend()->sendPresence(presence);
	}
}

void Buddy::generateJID() {
	m_jid = Swift::JID();
	m_jid = Swift::JID(getSafeName(), m_rosterManager->getUser()->getComponent()->getJID().toString(), "bot");
}

void Buddy::setID(long id) {
	m_id = id;
}

long Buddy::getID() {
	return m_id;
}

void Buddy::setFlags(BuddyFlag flags) {
	m_flags = flags;

	if (!getSafeName().empty()) {
		try {
			generateJID();
		} catch (...) {
		}
	}
}

BuddyFlag Buddy::getFlags() {
	return m_flags;
}

const Swift::JID &Buddy::getJID() {
	if (!m_jid.isValid() || m_jid.getNode().empty()) {
		generateJID();
	}
	return m_jid;
}

void Buddy::setSubscription(Subscription subscription) {
	m_subscription = subscription;
}

Buddy::Subscription Buddy::getSubscription() {
	return m_subscription;
}

void Buddy::handleRawPresence(Swift::Presence::ref presence) {
	for (std::vector<Swift::Presence::ref>::iterator it = m_presences.begin(); it != m_presences.end(); it++) {
		if ((*it)->getFrom() == presence->getFrom()) {
			m_presences.erase(it);
			break;
		}
	}

	m_presences.push_back(presence);
	m_rosterManager->getUser()->getComponent()->getFrontend()->sendPresence(presence);
}

std::vector<Swift::Presence::ref> &Buddy::generatePresenceStanzas(int features, bool only_new) {
	if (m_jid.getNode().empty()) {
		generateJID();
	}

	Swift::StatusShow s;
	std::string statusMessage;
	if (!getStatus(s, statusMessage)) {
		for (std::vector<Swift::Presence::ref>::iterator it = m_presences.begin(); it != m_presences.end(); it++) {
			if ((*it)->getFrom() == m_jid) {
				m_presences.erase(it);
				break;
			}
		}
		return m_presences;
	}

	Swift::Presence::ref presence = Swift::Presence::create();
	presence->setTo(m_rosterManager->getUser()->getJID().toBare());
	presence->setFrom(m_jid);
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	if (s.getType() == Swift::StatusShow::None)
		presence->setType(Swift::Presence::Unavailable);

	presence->setShow(s.getType());

	if (presence->getType() != Swift::Presence::Unavailable) {
		// caps
	

// 		if (features & 0/*TRANSPORT_FEATURE_AVATARS*/) {
			presence->addPayload(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (getIconHash())));
// 		}
// 		if (isBlocked()) {
// 			presence->addPayload(std::shared_ptr<Swift::Payload>(new Transport::BlockPayload ()));
// 		}
	}

	BOOST_FOREACH(Swift::Presence::ref &p, m_presences) {
		if (p->getFrom() == presence->getFrom()) {
			p = presence;
			return m_presences;
		}
	}

	m_presences.push_back(presence);

// 	if (only_new) {
// 		if (m_lastPresence)
// 			m_lastPresence->setTo(Swift::JID(""));
// 		if (m_lastPresence == presence) {
// 			return Swift::Presence::ref();
// 		}
// 		m_lastPresence = presence;
// 	}

	return m_presences;
}

std::string Buddy::getSafeName() {
	if (m_jid.isValid()) {
		return m_jid.getNode();
	}
	std::string name = getName();
// 	Transport::instance()->protocol()->prepareUsername(name, purple_buddy_get_account(m_buddy));
	if (getFlags() & BUDDY_JID_ESCAPING) {
		name = Swift::JID::getEscapedNode(name);
	}
	else {
		if (name.find_last_of("@") != std::string::npos) {
			name.replace(name.find_last_of("@"), 1, "%"); // OK
		}
	}
// 	if (name.empty()) {
// 		Log("SpectrumBuddy::getSafeName", "Name is EMPTY! Previous was " << getName() << ".");
// 	}
	return name;
}

void Buddy::handleVCardReceived(const std::string &id, Swift::VCard::ref vcard) {
	m_rosterManager->getUser()->getComponent()->getFrontend()->sendVCard(vcard, m_rosterManager->getUser()->getJID());
}

std::string Buddy::JIDToLegacyName(const Swift::JID &jid, User *user) {
	std::string name;
	if (jid.getUnescapedNode() == jid.getNode()) {
		name = jid.getNode();
		if (name.find_last_of("%") != std::string::npos) {
			name.replace(name.find_last_of("%"), 1, "@"); // OK
		}
	}
	else {
		name = jid.getUnescapedNode();
	}

	// If we have User associated with this request, we will find the
	// buddy in his Roster, because JID received from network can be lower-case
	// version of the original legacy name, but in the roster, we store the
	// case-sensitive version of the legacy name.
	if (user) {
		Buddy *b = user->getRosterManager()->getBuddy(name);
		if (b) {
			return b->getName();
		}
	}

	return name;
}

Buddy *Buddy::JIDToBuddy(const Swift::JID &jid, User *user) {
	std::string name;
	if (jid.getUnescapedNode() == jid.getNode()) {
		name = jid.getNode();
		if (name.find_last_of("%") != std::string::npos) {
			name.replace(name.find_last_of("%"), 1, "@"); // OK
		}
	}
	else {
		name = jid.getUnescapedNode();
	}

	// If we have User associated with this request, we will find the
	// buddy in his Roster, because JID received from network can be lower-case
	// version of the original legacy name, but in the roster, we store the
	// case-sensitive version of the legacy name.
	if (user) {
		Buddy *b = user->getRosterManager()->getBuddy(name);
		if (b) {
			return b;
		}
	}

	return NULL;
}

BuddyFlag Buddy::buddyFlagsFromJID(const Swift::JID &jid) {
	if (jid.getUnescapedNode() == jid.getNode()) {
		return BUDDY_NO_FLAG;
	}
	return BUDDY_JID_ESCAPING;
}

}
