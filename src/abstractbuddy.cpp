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

#include "transport/abstractbuddy.h"
#include "transport/rostermanager.h"
#include "transport/user.h"
#include "transport/transport.h"

namespace Transport {

AbstractBuddy::AbstractBuddy(RosterManager *rosterManager, long id) : m_id(id), m_online(false), m_subscription("ask"), m_flags(BUDDY_NO_FLAG), m_rosterManager(rosterManager){
	m_rosterManager->setBuddy(this);
}

AbstractBuddy::~AbstractBuddy() {
	m_rosterManager->unsetBuddy(this);
}

void AbstractBuddy::generateJID() {
	m_jid = Swift::JID();
	m_jid = Swift::JID(getSafeName(), m_rosterManager->getUser()->getComponent()->getJID().toString(), "bot");
}

void AbstractBuddy::setID(long id) {
	m_id = id;
}

long AbstractBuddy::getID() {
	return m_id;
}

void AbstractBuddy::setFlags(BuddyFlag flags) {
	m_flags = flags;

	generateJID();
}

BuddyFlag AbstractBuddy::getFlags() {
	return m_flags;
}

const Swift::JID &AbstractBuddy::getJID() {
	if (!m_jid.isValid()) {
		generateJID();
	}
	return m_jid;
}

void AbstractBuddy::setOnline() {
	m_online = true;
}

void AbstractBuddy::setOffline() {
	m_online = false;
	m_lastPresence = Swift::Presence::ref();
}

bool AbstractBuddy::isOnline() {
	return m_online;
}

void AbstractBuddy::setSubscription(const std::string &subscription) {
	m_subscription = subscription;
}

const std::string &AbstractBuddy::getSubscription() {
	return m_subscription;
}

Swift::Presence::ref AbstractBuddy::generatePresenceStanza(int features, bool only_new) {
	std::string alias = getAlias();
	std::string name = getSafeName();

	Swift::StatusShow s;
	std::string statusMessage;
	if (!getStatus(s, statusMessage))
		return Swift::Presence::ref();

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
// 		presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::CapsInfo (CONFIG().caps)));

		if (features & 0/*TRANSPORT_FEATURE_AVATARS*/) {
			presence->addPayload(boost::shared_ptr<Swift::Payload>(new Swift::VCardUpdate (getIconHash())));
		}
	}

	if (only_new) {
		if (m_lastPresence)
			m_lastPresence->setTo(Swift::JID(""));
		if (m_lastPresence == presence) {
			return Swift::Presence::ref();
		}
		m_lastPresence = presence;
	}

	return presence;
}

std::string AbstractBuddy::getSafeName() {
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
			name.replace(name.find_last_of("@"), 1, "%");
		}
	}
// 	if (name.empty()) {
// 		Log("SpectrumBuddy::getSafeName", "Name is EMPTY! Previous was " << getName() << ".");
// 	}
	return name;
}

void AbstractBuddy::buddyChanged() {
	Swift::Presence::ref presence = generatePresenceStanza(255);
	if (presence) {
		m_rosterManager->getUser()->getComponent()->getStanzaChannel()->sendPresence(presence);
	}
}

}
