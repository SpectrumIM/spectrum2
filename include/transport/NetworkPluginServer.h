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
#include <list>
#include <vector>
#include "Swiften/Network/BoostConnectionServer.h"
#include "Swiften/Network/Connection.h"
#include "Swiften/Network/Timer.h"
#include "transport/protocol.pb.h"

#define NETWORK_PLUGIN_API_VERSION (1)

namespace Transport {

class UserManager;
class User;
class Component;
class Buddy;
class LocalBuddy;
class Config;
class Conversation;
class AdminInterface;

class NetworkPluginServer {
	public:
		struct Backend {
			int pongReceived;
			std::list<User *> users;
			Swift::SafeByteArray data;
			std::shared_ptr<Swift::Connection> connection;
			unsigned long res;
			unsigned long init_res;
			unsigned long shared;
			bool acceptUsers;
			bool longRun;
			bool willDie;
			std::string id;
		};

		NetworkPluginServer(Component *component, Config *config, UserManager *userManager);

		virtual ~NetworkPluginServer();

		void start();

		void setAdminInterface(AdminInterface *adminInterface) {
			m_adminInterface = adminInterface;
		}

		int getBackendCount() {
			return m_clients.size();
		}

		const std::list<Backend *> &getBackends() {
			return m_clients;
		}

		const std::vector<std::string> &getCrashedBackends() {
			return m_crashedBackends;
		}

		void collectBackend();

		bool moveToLongRunBackend(User *user);

		void handleMessageReceived(Conversation *conv, pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &message);

	public:
		void handleNewClientConnection(std::shared_ptr<Swift::Connection> c);
		void handleSessionFinished(Backend *c);
		void handlePongReceived(Backend *c);
		void handleDataRead(Backend *c, std::shared_ptr<Swift::SafeByteArray> data);

		void handleConnectedPayload(const std::string &payload);
		void handleDisconnectedPayload(const std::string &payload);
		void handleBuddyChangedPayload(const std::string &payload);
		void handleBuddyRemovedPayload(const std::string &payload);
		void handleConvMessagePayload(const std::string &payload, bool subject = false);
		void handleConvMessageAckPayload(const std::string &payload);
		void handleParticipantChangedPayload(const std::string &payload);
		void handleRoomChangedPayload(const std::string &payload);
		void handleVCardPayload(const std::string &payload);
		void handleChatStatePayload(const std::string &payload, pbnetwork::WrapperMessage_Type type);
		void handleAuthorizationPayload(const std::string &payload);
		void handleAttentionPayload(const std::string &payload);
		void handleStatsPayload(Backend *c, const std::string &payload);
		void handleQueryPayload(Backend *b, const std::string &payload);
		void handleBackendConfigPayload(const std::string &payload);
		void handleRoomListPayload(const std::string &payload);
		void handleRawXML(const std::string &xml);

		void handleUserCreated(User *user);
		void handleRoomJoined(User *user, const std::string &room, const std::string &nickname, const std::string &password);
		void handleRoomLeft(User *user, const std::string &room);
		void handleUserReadyToConnect(User *user);
		void handleUserStatusChanged(User *user, const pbnetwork::Status &status);
		void handleUserDestroyed(User *user);

		void handleBuddyUpdated(Buddy *buddy, const pbnetwork::Buddy &item);
		void handleBuddyRemoved(Buddy *buddy);
		void handleBuddyAdded(Buddy *buddy, const pbnetwork::Buddy &item);
		void handleUserBuddyAdded(User *user, Buddy *buddy);
		void handleUserBuddyRemoved(User *user, Buddy *buddy);

		void handleBlockToggled(Buddy *buddy);

		void handleVCardUpdated(User *user, const pbnetwork::VCard &vcard);
		void handleVCardRequired(User *user, const std::string &name, unsigned int id);

		void handlePIDTerminated(unsigned long pid);

	private:
		void send(std::shared_ptr<Swift::Connection> &, const std::string &data);
		/// Sends the wrapped payload to the backend of user, if the user has one.
		void sendToUserBackend(User *user, pbnetwork::WrapperMessage_Type type, const std::string &payload);

		void pingTimeout();
		void sendPing(Backend *c);
		void sendAPIVersion(Backend *c);
		Backend *getFreeClient(bool acceptUsers = true, bool longRun = false, bool check = false);
		void connectWaitingUsers();
		void loginDelayFinished();
		void handleRawXMLReceived(User *user, const std::string &xml);
		/// Returns the user's conversation with name, creating it when needed.
		Conversation *getOrCreateConversation(User *user, const std::string &name);

		UserManager *m_userManager;
		Config *m_config;
		std::shared_ptr<Swift::ConnectionServer> m_server;
		std::list<Backend *>  m_clients;
		std::vector<unsigned long> m_pids;
		Swift::Timer::ref m_pingTimer;
		Swift::Timer::ref m_collectTimer;
		Swift::Timer::ref m_loginTimer;
		Component *m_component;
		std::list<User *> m_waitingUsers;
		bool m_isNextLongRun;
		std::vector<std::string> m_crashedBackends;
		AdminInterface *m_adminInterface;
		bool m_startingBackend;
		time_t m_lastLogin;
		bool m_firstPong;
};

}
