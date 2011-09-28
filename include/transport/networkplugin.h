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
#include "conversation.h"

namespace Transport {

/// Represents Spectrum2 legacy network plugin.

/// This class is base class for all C++ legacy network plugins. It provides a way to connect 
/// Spectrum2 NetworkPluginServer and allows to use high-level API for legacy network plugins
/// development.
class NetworkPlugin {
	public:
		/// Creates new NetworkPlugin and connects the Spectrum2 NetworkPluginServer.
		/// \param loop Event loop.
		/// \param host Host where Spectrum2 NetworkPluginServer runs.
		/// \param port Port.
		NetworkPlugin(Swift::EventLoop *loop, const std::string &host, int port);

		/// Destructor.
		virtual ~NetworkPlugin();
		
		virtual void readyForData() {}

		/// Call this function when legacy network buddy changed.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param buddyName Name of legacy network buddy. (eg. "user2@gmail.com")
		/// \param alias Alias of legacy network buddy. If empty, then it's not changed on XMPP side.
		/// \param groups Groups in which buddy currently is. If empty, then it's not changed on XMPP side.
		/// \param status Status of this buddy.
		/// \param statusMessage Status message of this buddy.
		/// \param iconHash MD5 hash of buddy icon. Empty if none buddy icon.
		/// \param blocked True if this buddy is blocked in privacy lists in legacy network.
		void handleBuddyChanged(const std::string &user, const std::string &buddyName, const std::string &alias,
			const std::string &groups, Swift::StatusShow::Type status, const std::string &statusMessage = "", const std::string &iconHash = "",
			bool blocked = false
		);

		/// Call this function when participant in room changed.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param nickname Nickname of participant. If participant renamed, this is old name of participant. (eg. "HanzZ")
		/// \param room Room in which participant changed. (eg. #spectrum)
		/// \param flags Participant flags.
		/// \param status Current status of participant. Swift::StatusShow::None if participant left the room.
		/// \param statusMessage Current status message of participant.
		/// \param newname New name of participant if he changed the nickname. Otherwise empty.
		void handleParticipantChanged(const std::string &user, const std::string &nickname, const std::string &room, Conversation::ParticipantFlag flags = Conversation::None,
			Swift::StatusShow::Type status = Swift::StatusShow::None, const std::string &statusMessage = "", const std::string &newname = "");

		/// Call this function when user disconnected the legacy network because of some legacy network error.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param error Reserved for future use, currently keep it on 0.
		/// \param message XMPP message which is sent to XMPP user.
		void handleDisconnected(const std::string &user, int error = 0, const std::string &message = "");

		/// Call this function when user connected the legacy network and is logged in.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		void handleConnected(const std::string &user);

		/// Call this function when new message is received from legacy network for user.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param legacyName Name of legacy network buddy or name of room. (eg. "user2@gmail.com")
		/// \param message Plain text message.
		/// \param nickname Nickname of buddy in room. Empty if it's normal chat message.
		/// \param xhtml XHTML message.
		void handleMessage(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &nickname = "", const std::string &xhtml = "");

		/// Call this function when subject in room changed.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param legacyName Name of room. (eg. "#spectrum")
		/// \param message Subject message.
		/// \param nickname Nickname of user who changed subject.
		void handleSubject(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &nickname = "");

		/// Call this function XMPP user's nickname changed.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param room Room in which participant changed. (eg. #spectrum)
		/// \param nickname New nickname.
		void handleRoomNicknameChanged(const std::string &user, const std::string &room, const std::string &nickname);

		/// Call this function when requested VCard arrived.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param id VCard ID.
		/// \param legacyName Name of legacy network buddy. (eg. "user2@gmail.com")
		/// \param fullName Name of legacy network buddy. (eg. "Monty Python")
		/// \param nickname Nickname.
		/// \param photo Raw photo.
		void handleVCard(const std::string &user, unsigned int id, const std::string &legacyName, const std::string &fullName, const std::string &nickname, const std::string &photo);

		/// Call this function when buddy started typing.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param buddyName Name of legacy network buddy. (eg. "user2@gmail.com")
		void handleBuddyTyping(const std::string &user, const std::string &buddyName);

		/// Call this function when buddy typed, but is not typing anymore.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param buddyName Name of legacy network buddy. (eg. "user2@gmail.com")
		void handleBuddyTyped(const std::string &user, const std::string &buddyName);

		/// Call this function when buddy has been typing, but paused for a while.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param buddyName Name of legacy network buddy. (eg. "user2@gmail.com")
		void handleBuddyStoppedTyping(const std::string &user, const std::string &buddyName);

		/// Call this function when new authorization request arrived form legacy network
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param buddyName Name of legacy network buddy. (eg. "user2@gmail.com")
		void handleAuthorization(const std::string &user, const std::string &buddyName);

		/// Call this function when attention request arrived from legacy network.
		/// \param user XMPP JID of user for which this event occurs. You can get it from NetworkPlugin::handleLoginRequest(). (eg. "user%gmail.com@xmpp.domain.tld")
		/// \param buddyName Name of legacy network buddy. (eg. "user2@gmail.com")
		/// \param message Message.
		void handleAttention(const std::string &user, const std::string &buddyName, const std::string &message);

		void handleFTStart(const std::string &user, const std::string &buddyName, const std::string fileName, unsigned long size);

		void handleFTData(unsigned long ftID, const std::string &data);

		/// Called when XMPP user wants to connect legacy network.
		/// You should connect him to legacy network and call handleConnected or handleDisconnected function later.
		/// \param user XMPP JID of user for which this event occurs.
		/// \param legacyName Legacy network name of this user used for login.
		/// \param password Legacy network password of this user.
		/**
			\msc
			NetworkPlugin,YourNetworkPlugin,LegacyNetwork;
			NetworkPlugin->YourNetworkPlugin [label="handleLoginRequest(...)", URL="\ref NetworkPlugin::handleLoginRequest()"];
			YourNetworkPlugin->LegacyNetwork [label="connect the legacy network"];
			--- [label="If password was valid and user is connected and logged in"];
			YourNetworkPlugin<-LegacyNetwork [label="connected"];
			YourNetworkPlugin->NetworkPlugin [label="handleConnected()", URL="\ref NetworkPlugin::handleConnected()"];
			--- [label="else"];
			YourNetworkPlugin<-LegacyNetwork [label="disconnected"];
			YourNetworkPlugin->NetworkPlugin [label="handleDisconnected()", URL="\ref NetworkPlugin::handleDisconnected()"];
			\endmsc
		*/
		virtual void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) = 0;

		/// Called when XMPP user wants to disconnect legacy network.
		/// You should disconnect him from legacy network.
		/// \param user XMPP JID of user for which this event occurs.
		/// \param legacyName Legacy network name of this user used for login.
		virtual void handleLogoutRequest(const std::string &user, const std::string &legacyName) = 0;

		/// Called when XMPP user sends message to legacy network.
		/// \param user XMPP JID of user for which this event occurs.
		/// \param legacyName Legacy network name of buddy or room.
		/// \param message Plain text message.
		/// \param xhtml XHTML message.
		virtual void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") = 0;

		/// Called when XMPP user requests VCard of buddy.
		/// \param user XMPP JID of user for which this event occurs.
		/// \param legacyName Legacy network name of buddy whose VCard is requested.
		/// \param id ID which is associated with this request. You have to pass it to handleVCard function when you receive VCard.
		/**
			\msc
			NetworkPlugin,YourNetworkPlugin,LegacyNetwork;
			NetworkPlugin->YourNetworkPlugin [label="handleVCardRequest(...)", URL="\ref NetworkPlugin::handleVCardRequest()"];
			YourNetworkPlugin->LegacyNetwork [label="start VCard fetching"];
			YourNetworkPlugin<-LegacyNetwork [label="VCard fetched"];
			YourNetworkPlugin->NetworkPlugin [label="handleVCard()", URL="\ref NetworkPlugin::handleVCard()"];
			\endmsc
		*/
		virtual void handleVCardRequest(const std::string &/*user*/, const std::string &/*legacyName*/, unsigned int /*id*/) {}

		/// Called when XMPP user updates his own VCard.
		/// You should update the VCard in legacy network too.
		/// \param user XMPP JID of user for which this event occurs.
		/// \param photo Raw photo data.
		virtual void handleVCardUpdatedRequest(const std::string &/*user*/, const std::string &/*photo*/) {}


		virtual void handleJoinRoomRequest(const std::string &/*user*/, const std::string &/*room*/, const std::string &/*nickname*/, const std::string &/*pasword*/) {}
		virtual void handleLeaveRoomRequest(const std::string &/*user*/, const std::string &/*room*/) {}
		virtual void handleStatusChangeRequest(const std::string &/*user*/, int status, const std::string &statusMessage) {}
		virtual void handleBuddyUpdatedRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*alias*/, const std::string &/*groups*/) {}
		virtual void handleBuddyRemovedRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*groups*/) {}
		virtual void handleBuddyBlockToggled(const std::string &/*user*/, const std::string &/*buddyName*/, bool /*blocked*/) {}

		virtual void handleTypingRequest(const std::string &/*user*/, const std::string &/*buddyName*/) {}
		virtual void handleTypedRequest(const std::string &/*user*/, const std::string &/*buddyName*/) {}
		virtual void handleStoppedTypingRequest(const std::string &/*user*/, const std::string &/*buddyName*/) {}
		virtual void handleAttentionRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*message*/) {}

		virtual void handleFTStartRequest(const std::string &/*user*/, const std::string &/*buddyName*/, const std::string &/*fileName*/, unsigned long size, unsigned long ftID) {}

		virtual void handleExit() { std::cout << "EXITING\n"; exit(1); }
		

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
		void handleFTStartPayload(const std::string &payload);
		void handleDataRead(const Swift::SafeByteArray&);
		void _handleConnected(bool error);
		void handleDisconnected();

		void send(const std::string &data);
		void sendPong();
		void sendMemoryUsage();
		void pingTimeout();

		Swift::SafeByteArray m_data;
		std::string m_host;
		int m_port;
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;
		Swift::EventLoop *m_loop;
		Swift::Timer::ref m_pingTimer;
		bool m_pingReceived;
		double m_init_res;

};

}
