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
#include "Swiften/Network/BoostConnectionServer.h"
#include "Swiften/Network/Connection.h"
#include "storagebackend.h"
#include "transport/filetransfermanager.h"

namespace Transport {

class UserManager;
class User;
class Component;
class Buddy;
class LocalBuddy;
class Config;
class NetworkConversation;
class VCardResponder;
class RosterResponder;
class BlockResponder;
class DummyReadBytestream;
class AdminInterface;
class DiscoItemsResponder;

class NetworkPluginServer {
	public:
		struct Backend {
			int pongReceived;
			std::list<User *> users;
			Swift::SafeByteArray data;
			boost::shared_ptr<Swift::Connection> connection;
			unsigned long res;
			unsigned long init_res;
			unsigned long shared;
			bool acceptUsers;
			bool longRun;
			bool willDie;
			std::string id;
		};

		NetworkPluginServer(Component *component, Config *config, UserManager *userManager, FileTransferManager *ftManager, DiscoItemsResponder *discoItemsResponder);

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

		void handleMessageReceived(NetworkConversation *conv, boost::shared_ptr<Swift::Message> &message);

	public:
		void handleNewClientConnection(boost::shared_ptr<Swift::Connection> c);
		void handleSessionFinished(Backend *c);
		void handlePongReceived(Backend *c);
		void handleDataRead(Backend *c, boost::shared_ptr<Swift::SafeByteArray> data);

		void handleConnectedPayload(const std::string &payload);
		void handleDisconnectedPayload(const std::string &payload);
		void handleBuddyChangedPayload(const std::string &payload);
		void handleBuddyRemovedPayload(const std::string &payload);
		void handleConvMessagePayload(const std::string &payload, bool subject = false);
		void handleConvMessageAckPayload(const std::string &payload);
		void handleParticipantChangedPayload(const std::string &payload);
		void handleRoomChangedPayload(const std::string &payload);
		void handleVCardPayload(const std::string &payload);
		void handleChatStatePayload(const std::string &payload, Swift::ChatState::ChatStateType type);
		void handleAuthorizationPayload(const std::string &payload);
		void handleAttentionPayload(const std::string &payload);
		void handleStatsPayload(Backend *c, const std::string &payload);
		void handleFTStartPayload(const std::string &payload);
		void handleFTFinishPayload(const std::string &payload);
		void handleFTDataPayload(Backend *b, const std::string &payload);
		void handleQueryPayload(Backend *b, const std::string &payload);
		void handleBackendConfigPayload(const std::string &payload);
		void handleRoomListPayload(const std::string &payload);

		void handleUserCreated(User *user);
		void handleRoomJoined(User *user, const Swift::JID &who, const std::string &room, const std::string &nickname, const std::string &password);
		void handleRoomLeft(User *user, const std::string &room);
		void handleUserReadyToConnect(User *user);
		void handleUserPresenceChanged(User *user, Swift::Presence::ref presence);
		void handleUserDestroyed(User *user);

		void handleBuddyUpdated(Buddy *buddy, const Swift::RosterItemPayload &item);
		void handleBuddyRemoved(Buddy *buddy);
		void handleBuddyAdded(Buddy *buddy, const Swift::RosterItemPayload &item);

		void handleBlockToggled(Buddy *buddy);

		void handleVCardUpdated(User *user, boost::shared_ptr<Swift::VCard> vcard);
		void handleVCardRequired(User *user, const std::string &name, unsigned int id);

		void handleFTStateChanged(Swift::FileTransfer::State state, const std::string &userName, const std::string &buddyName, const std::string &fileName, unsigned long size, unsigned long id);
		void handleFTAccepted(User *user, const std::string &buddyName, const std::string &fileName, unsigned long size, unsigned long ftID);
		void handleFTRejected(User *user, const std::string &buddyName, const std::string &fileName, unsigned long size);
		void handleFTDataNeeded(Backend *b, unsigned long ftid);

	private:
		void send(boost::shared_ptr<Swift::Connection> &, const std::string &data);

		void pingTimeout();
		void sendPing(Backend *c);
		Backend *getFreeClient(bool acceptUsers = true, bool longRun = false, bool check = false);
		void connectWaitingUsers();
		void loginDelayFinished();

		UserManager *m_userManager;
		VCardResponder *m_vcardResponder;
		RosterResponder *m_rosterResponder;
		BlockResponder *m_blockResponder;
		Config *m_config;
		boost::shared_ptr<Swift::ConnectionServer> m_server;
		std::list<Backend *>  m_clients;
		Swift::Timer::ref m_pingTimer;
		Swift::Timer::ref m_collectTimer;
		Swift::Timer::ref m_loginTimer;
		Component *m_component;
		std::list<User *> m_waitingUsers;
		bool m_isNextLongRun;
		std::map<unsigned long, FileTransferManager::Transfer> m_filetransfers;
		FileTransferManager *m_ftManager;
		std::vector<std::string> m_crashedBackends;
		AdminInterface *m_adminInterface;
		bool m_startingBackend;
		DiscoItemsResponder *m_discoItemsResponder;
		time_t m_lastLogin;
};

}
