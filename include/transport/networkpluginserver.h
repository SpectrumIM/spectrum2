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
#include "storagebackend.h"

namespace Transport {

class UserManager;
class User;
class Component;
class Buddy;
class LocalBuddy;
class Config;
class NetworkConversation;

class NetworkPluginServer {
	public:
		struct Client {
			bool pongReceived;
			std::list<User *> users;
			std::string data;
			boost::shared_ptr<Swift::Connection> connection;
		};

		NetworkPluginServer(Component *component, Config *config, UserManager *userManager);

		virtual ~NetworkPluginServer();

		void handleMessageReceived(NetworkConversation *conv, boost::shared_ptr<Swift::Message> &message);

	private:
		void handleNewClientConnection(boost::shared_ptr<Swift::Connection> c);
		void handleSessionFinished(Client *c);
		void handleDataRead(Client *c, const Swift::ByteArray&);

		void handleConnectedPayload(const std::string &payload);
		void handleDisconnectedPayload(const std::string &payload);
		void handleBuddyChangedPayload(const std::string &payload);
		void handleConvMessagePayload(const std::string &payload, bool subject = false);
		void handleParticipantChangedPayload(const std::string &payload);
		void handleRoomChangedPayload(const std::string &payload);

		void handleUserCreated(User *user);
		void handleRoomJoined(User *user, const std::string &room, const std::string &nickname, const std::string &password);
		void handleRoomLeft(User *user, const std::string &room);
		void handleUserReadyToConnect(User *user);
		void handleUserDestroyed(User *user);

		void send(boost::shared_ptr<Swift::Connection> &, const std::string &data);

		void pingTimeout();
		void sendPing(Client *c);
		Client *getFreeClient();

		UserManager *m_userManager;
		Config *m_config;
		boost::shared_ptr<Swift::ConnectionServer> m_server;
		std::list<Client *>  m_clients;
		Swift::Timer::ref m_pingTimer;
};

}
