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
#include "Swiften/Swiften.h"

namespace Transport {

User::User(const Swift::JID &jid, const std::string &username, const std::string &password, Component *component) {
	m_jid = jid;

	m_component = component->m_component;
	m_presenceOracle = component->m_presenceOracle;
	m_entityCapsManager = component->m_entityCapsManager;
// 	m_activeResource = m_jid.getResource();

}
User::~User(){

}

const Swift::JID &User::getJID() {
	return m_jid;
}

}
