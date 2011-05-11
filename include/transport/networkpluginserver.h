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

class NetworkPluginServer {
	public:
		NetworkPluginServer(Component *component, Config *config, UserManager *userManager);

		virtual ~NetworkPluginServer();

	private:
		void handleNewClientConnection(boost::shared_ptr<Swift::Connection> c);
		void handleSessionFinished(boost::shared_ptr<Swift::Connection>);
		void handleDataRead(boost::shared_ptr<Swift::Connection>, const Swift::ByteArray&);

		void handleConnectedPayload(const std::string &payload);
		void handleDisconnectedPayload(const std::string &payload);
		void handleBuddyChangedPayload(const std::string &payload);

		void handleUserCreated(User *user);
		void handleUserReadyToConnect(User *user);
		void handleUserDestroyed(User *user);

		void send(boost::shared_ptr<Swift::Connection> &, const std::string &data);

		std::string m_command;
		std::string m_data;
		UserManager *m_userManager;
		Config *m_config;
		boost::shared_ptr<Swift::ConnectionServer> m_server;
		boost::shared_ptr<Swift::Connection> m_client;
};

}
