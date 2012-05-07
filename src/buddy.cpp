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

#include "transport/buddy.h"
#include "transport/rostermanager.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/BlockPayload.h"

namespace Transport {

Buddy::Buddy(RosterManager *rosterManager, long id) : m_id(id), m_flags(BUDDY_NO_FLAG), m_rosterManager(rosterManager),
	m_subscription(Ask) {
// 	m_rosterManager->setBuddy(this);
}

Buddy::~Buddy() {
// 	m_rosterManager->unsetBuddy(this);
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

// 	generateJID();
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

Swift::Presence::ref Buddy::generatePresenceStanza(int features, bool only_new) {
	std::string alias = getAlias();
	std::string name = getSafeName();

	Swift::StatusShow s;
	std::string statusMessage;
	if (!getStatus(s, statusMessage))
		return Swift::Presence::ref();

	if (m_jid.getNode().empty()) {
		generateJID();
	}

	Swift::Presence::ref presence = Swift::Presence::create();
 	presence->setFrom(m_jid);
	presence->setTo(m_rosterManager->getUser()->getJID().toBare());
	presence->setType(Swift::Presence::Available);

	if (!statusMessage.empty())
		presence->setStatus(statusMessage);

	if (s.getType() == Swift::StatusShow::None)
		presence->setType(Swift::Presence::Unavailable);
	presence->setShow(s.getType());

	if (presence->getType() != Swift::Presence::Unavailable) {
		// caps
		
		presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::CapsInfo(m_rosterManager->getUser()->getComponent()->getBuddyCapsInfo())));

// 		if (features & 0/*TRANSPORT_FEATURE_AVATARS*/) {
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (getIconHash())));
// 		}
		if (isBlocked()) {
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Transport::BlockPayload ()));
		}
	}

// 	if (only_new) {
// 		if (m_lastPresence)
// 			m_lastPresence->setTo(Swift::JID(""));
// 		if (m_lastPresence == presence) {
// 			return Swift::Presence::ref();
// 		}
// 		m_lastPresence = presence;
// 	}

	return presence;
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

void Buddy::handleBuddyChanged() {
	Swift::Presence::ref presence = generatePresenceStanza(255);
	if (presence) {
		m_rosterManager->getUser()->getComponent()->getStanzaChannel()->sendPresence(presence);
	}
}

void Buddy::handleVCardReceived(const std::string &id, Swift::VCard::ref vcard) {
	boost::shared_ptr<Swift::GenericRequest<Swift::VCard> > request(new Swift::GenericRequest<Swift::VCard>(Swift::IQ::Result, m_rosterManager->getUser()->getJID(), vcard, m_rosterManager->getUser()->getComponent()->getIQRouter()));
	request->send();
}

std::string Buddy::JIDToLegacyName(const Swift::JID &jid) {
	std::string name;
	if (jid.getUnescapedNode() == jid.getNode()) {
		name = jid.getNode();
		if (name.find_last_of("%") != std::string::npos) {
			name.replace(name.find_last_of("%"), 1, "@"); // OK
		}
	}
	else {
		name = jid.getUnescapedNode();
		// Psi sucks...
// 		if (name.find_last_of("\\40") != std::string::npos) {
// 			name.replace(name.find_last_of("\\40"), 1, "@"); // OK
// 		}
	}
	return name;
}

BuddyFlag Buddy::buddFlagsFromJID(const Swift::JID &jid) {
	if (jid.getUnescapedNode() == jid.getNode()) {
		return BUDDY_NO_FLAG;
	}
	return BUDDY_JID_ESCAPING;
}

}
