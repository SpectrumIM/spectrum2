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
#include <list>
#include <map>
#include <vector>
#include "Swiften/JID/JID.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/DiscoItems.h"
#include "Swiften/Network/Timer.h"
#include "Swiften/VCards/GetVCardRequest.h"

namespace Transport {

class Component;
class UserManager;
class XMPPFrontend;
struct UserInfo;

/// Represents online XMPP user.
class XMPPUser : public User {
	public:
		/// Creates new User class.
		/// \param user bare JID of the XMPP user
		/// \param userInfo UserInfo struct with informations needed to connect
		/// this user to legacy network
		/// \param component Component associated with this user
		XMPPUser(const std::string &user, UserInfo &userInfo, Component * component, UserManager *userManager);

		/// Destroyes User.
		virtual ~XMPPUser();

		/// Returns bare JID of the user.
		const Swift::JID &getJID() { return m_jid; }

		/// Returns full JIDs of the user's resources which support particular feature.
		/// \param feature disco#info feature.
		std::vector<Swift::JID> getJIDWithFeature(const std::string &feature);

		void disconnectUser(const std::string &error, pbnetwork::ConnectionError e);

		void requestVCard();

		void clearRoomList();

		void addRoomToRoomList(const std::string &room, const std::string &name);

		std::shared_ptr<Swift::DiscoItems> getRoomList() {
			return m_rooms;
		}

		void sendCurrentPresence();

		/// Handles presence from XMPP JID associated with this user.
		/// \param presence Swift::Presence.
		void handlePresence(Swift::Presence::ref presence);

		void handleSubscription(Swift::Presence::ref presence);

		void handleDiscoInfo(const Swift::JID& jid, std::shared_ptr<Swift::DiscoInfo> info);

		int getResourceCount() {
			return m_resources;
		}

		void leaveRoom(const std::string &room);

		/// Sends the next 1:1 messages to the user's bare JID again.
		void resetResources();

		/// The resource is gone: removes it from all conversations and leaves the rooms nobody is in.
		void removeJID(const Swift::JID &jid);

	protected:
		void handleRoomJoined(Conversation *conv);

	private:
		XMPPFrontend *getFrontend();
		void onConnectingTimeout();
		void handleVCardReceived(std::shared_ptr<Swift::VCard> vcard, Swift::ErrorPayload::ref error, Swift::GetVCardRequest::ref request);
		/// Sends the user's status (the presence of his highest priority resource) to the legacy network.
		void sendStatus(Swift::Presence::ref presence);
		void clearJIDs();

		Swift::JID m_jid;
		Component *m_component;
		UserManager *m_userManager;
		std::list <Swift::GetVCardRequest::ref> m_vcardRequests;
		std::shared_ptr<Swift::DiscoItems> m_rooms;
		Swift::Timer::ref m_reconnectTimer;
		std::map<Swift::JID, Swift::DiscoInfo::ref> m_legacyCaps;
		int m_resources;
		// The presence which joined each room, by room name.
		std::map<std::string, Swift::Presence::ref> m_joinedRooms;
};

}
