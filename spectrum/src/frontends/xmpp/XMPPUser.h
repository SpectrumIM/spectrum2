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

#include "transport/User.h"

#include <time.h>
#include "Swiften/Disco/EntityCapsManager.h"
#include "Swiften/Disco/EntityCapsProvider.h"
#include <Swiften/FileTransfer/OutgoingFileTransfer.h>
#include "Swiften/Elements/SpectrumErrorPayload.h"
#include "Swiften/Network/Timer.h"
#include "Swiften/Network/Connection.h"
#include "Swiften/VCards/GetVCardRequest.h"

namespace Transport {

class Component;
class RosterManager;
class ConversationManager;
class UserManager;
class PresenceOracle;
struct UserInfo;

/// Represents online XMPP user.
class XMPPUser : public User {
	public:
		/// Creates new User class.
		/// \param jid XMPP JID associated with this user
		/// \param userInfo UserInfo struct with informations needed to connect
		/// this user to legacy network
		/// \param component Component associated with this user
		XMPPUser(const Swift::JID &jid, UserInfo &userInfo, Component * component, UserManager *userManager);

		/// Destroyes User.
		virtual ~XMPPUser();

		void disconnectUser(const std::string &error, Swift::SpectrumErrorPayload::Error e);

		void requestVCard();

	private:
		void onConnectingTimeout();
		void handleVCardReceived(boost::shared_ptr<Swift::VCard> vcard, Swift::ErrorPayload::ref error, Swift::GetVCardRequest::ref request);

		Swift::JID m_jid;
		Component *m_component;
		UserManager *m_userManager;
		UserInfo m_userInfo;
		std::list <Swift::GetVCardRequest::ref> m_vcardRequests;
};

}
