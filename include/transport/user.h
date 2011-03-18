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

#pragma once

#include <time.h>
#include "Swiften/Swiften.h"
#include "Swiften/Presence/PresenceOracle.h"
#include "Swiften/Disco/EntityCapsManager.h"
#include "storagebackend.h"

namespace Transport {

class Component;
struct UserInfo;

/// Represents online XMPP user.
class User {
	public:
		/// Creates new User class.
		/// \param jid XMPP JID associated with this user
		/// \param userInfo UserInfo struct with informations needed to connect
		/// this user to legacy network
		/// \param component Component associated with this user
		User(const Swift::JID &jid, UserInfo &userInfo, Component * component);

		/// Destroyes User.
		virtual ~User();

		/// Returns JID of XMPP user who is currently connected using this User class.
		/// \return full JID
		const Swift::JID &getJID();

		/// Returns UserInfo struct with informations needed to connect the legacy network.
		/// \return UserInfo struct
		UserInfo &getUserInfo() { return m_userInfo; }

		void setData(void *data) { m_data = data; }
		void *getData() { return m_data; }

		/// Handles presence from XMPP JID associated with this user.
		/// \param presence Swift::Presence.
		void handlePresence(Swift::Presence::ref presence);

		/// Returns language.
		/// \return language
		const char *getLang() { return "en"; }

		boost::signal<void ()> onReadyToConnect;

	private:
		void onConnectingTimeout();

		Swift::JID m_jid;
		Component *m_component;		
		Swift::EntityCapsManager *m_entityCapsManager;
		Swift::PresenceOracle *m_presenceOracle;
		UserInfo m_userInfo;
		void *m_data;
		bool m_connected;
		bool m_readyForConnect;
		Swift::Timer::ref m_reconnectTimer;
};

}
