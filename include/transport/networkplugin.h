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
#include "Swiften/Network/ConnectionServer.h"
#include "Swiften/Network/Connection.h"
#include "Swiften/Network/BoostTimerFactory.h"
#include "Swiften/Network/BoostNetworkFactories.h"
#include "Swiften/Network/BoostIOServiceThread.h"
#include "Swiften/Network/Connection.h"
#include "storagebackend.h"

namespace Transport {

class NetworkPlugin {
	public:
		NetworkPlugin(Swift::EventLoop *loop, const std::string &host, int port);

		virtual ~NetworkPlugin();

		void handleBuddyChanged(const std::string &user, const std::string &buddyName, const std::string &alias,
			const std::string &groups, int status, const std::string &statusMessage = "", const std::string &iconHash = ""
		);

		void handleParticipantChanged(const std::string &user, const std::string &nickname, const std::string &room, int flags, int status = Swift::StatusShow::None, const std::string &statusMessage = "", const std::string &newname = "");

		void handleDisconnected(const std::string &user, const std::string &legacyName, int error, const std::string &message);

		void handleConnected(const std::string &user);

		void handleMessage(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &nickname = "", const std::string &xhtml = "");

		void handleSubject(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &nickname = "");

		void handleRoomChanged(const std::string &user, const std::string &room, const std::string &nickname);

		void handleVCard(const std::string &user, unsigned int id, const std::string &legacyName, const std::string &fullName, const std::string &nickname, const std::string &photo);

		void handleBuddyTyping(const std::string &user, const std::string &buddyName);
		
		void handleBuddyTyped(const std::string &user, const std::string &buddyName);

		void handleBuddyStoppedTyping(const std::string &user, const std::string &buddyName);

		void handleAuthorization(const std::string &user, const std::string &buddyName);

		void handleAttention(const std::string &user, const std::string &buddyName, const std::string &message);

		virtual void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) = 0;
		virtual void handleLogoutRequest(const std::string &user, const std::string &legacyName) = 0;
		virtual void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") = 0;
		virtual void handleVCardRequest(const std::string &/*user*/, const std::string &/*legacyName*/, unsigned int /*id*/) {}
		virtual void handleVCardUpdatedRequest(const std::string &/*user*/, const std::string &/*photo*/) {}
		virtual void handleJoinRoomRequest(const std::string &/*user*/, const std::string &/*room*/, const std::string &/*nickname*/, const std::string &/*pasword*/) {}
		virtual void handleLeaveRoomRequest(const std::string &/*user*/, const std::string &/*room*/) {}
		virtual void handleStatusChangeRequest(const std::string &/*user*/, int status, const std::string &statusMessage) {}
		virtual void handleBuddyUpdatedRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*alias*/, const std::string &/*groups*/) {}
		virtual void handleBuddyRemovedRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*groups*/) {}

		virtual void handleTypingRequest(const std::string &/*user*/, const std::string &/*buddyName*/) {}
		virtual void handleTypedRequest(const std::string &/*user*/, const std::string &/*buddyName*/) {}
		virtual void handleStoppedTypingRequest(const std::string &/*user*/, const std::string &/*buddyName*/) {}
		virtual void handleAttentionRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*message*/) {}
		

	private:
		void connect();
		void handleLoginPayload(const std::string &payload);
		void handleLogoutPayload(const std::string &payload);
		void handleStatusChangedPayload(const std::string &payload);
		void handleConvMessagePayload(const std::string &payload);
		void handleJoinRoomPayload(const std::string &payload);
		void handleLeaveRoomPayload(const std::string &payload);
		void handleVCardPayload(const std::string &payload);
		void handleBuddyChangedPayload(const std::string &payload);
		void handleBuddyRemovedPayload(const std::string &payload);
		void handleChatStatePayload(const std::string &payload, Swift::ChatState::ChatStateType type);
		void handleAttentionPayload(const std::string &payload);
		void handleDataRead(const Swift::SafeByteArray&);
		void _handleConnected(bool error);
		void handleDisconnected();

		void send(const std::string &data);
		void sendPong();
		void pingTimeout();

		Swift::SafeByteArray m_data;
		std::string m_host;
		int m_port;
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;
		Swift::Timer::ref m_pingTimer;
		bool m_pingReceived;

};

}
