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

namespace Transport {

AbstractBuddy::AbstractBuddy(long id) : m_id(id), m_online(false), m_subscription("ask"), m_flags(0) {
}

AbstractBuddy::~AbstractBuddy() {
}

void AbstractBuddy::setId(long id) {
	m_id = id;
}

long AbstractBuddy::getId() {
	return m_id;
}

void AbstractBuddy::setFlags(int flags) {
	m_flags = flags;
}

int AbstractBuddy::getFlags() {
	return m_flags;
}

Swift::JID AbstractBuddy::getJID(const std::string &hostname) {
 	return Swift::JID(getSafeName(), hostname, "bot");
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
// 	presence->setFrom(getJID());
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

}
