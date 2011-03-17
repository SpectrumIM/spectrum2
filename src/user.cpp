/**
 * libtransport -- C++ library for easy XMPP Transports development
 *
 * Copyright (C) 2011, Jan Kaluza <hanzz.k@gmail.com>
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

#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "Swiften/Swiften.h"

namespace Transport {

User::User(const Swift::JID &jid, UserInfo &userInfo, Component *component) {
	m_jid = jid;

	m_component = component;
	m_presenceOracle = component->m_presenceOracle;
	m_entityCapsManager = component->m_entityCapsManager;
	m_userInfo = userInfo;
}

User::~User(){

}

const Swift::JID &User::getJID() {
	return m_jid;
}

void User::handlePresence(Swift::Presence::ref presence) {
	Swift::Presence::ref highest = m_presenceOracle->getHighestPriorityPresence(m_jid.toBare());
	if (highest) {
		highest->setTo(presence->getFrom());
		highest->setFrom(m_component->getJID());
		m_component->getComponent()->sendPresence(highest);
	}
	else {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(m_jid.toBare());
		response->setFrom(m_component->getJID());
		response->setType(Swift::Presence::Unavailable);
		m_component->getComponent()->sendPresence(response);
	}
}

}
