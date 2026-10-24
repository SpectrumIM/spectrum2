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

#include "transport/User.h"
#include "transport/Frontend.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/UserManager.h"
#include "transport/Conversation.h"
#include "transport/Factory.h"
#include "transport/ConversationManager.h"
#include "transport/Logging.h"
#include "transport/StorageBackend.h"
#include <stdio.h>
#include <stdlib.h>

namespace Transport {

DEFINE_LOGGER(userLogger, "User");

User::User(const std::string &user, UserInfo &userInfo, Component *component, UserManager *userManager) {
	m_userName = user;
	m_data = NULL;

	m_cacheMessages = false;
	m_component = component;
	m_userManager = userManager;
	m_userInfo = userInfo;
	m_connected = false;
	m_readyForConnect = false;
	m_ignoreDisconnect = false;
	m_reconnectCounter = 0;
	m_reconnectLimit = 3;
	m_storageBackend = NULL;

	m_rosterManager = component->getFrontend()->createRosterManager(this, m_component);
	m_conversationManager = new ConversationManager(this, m_component);

	LOG4CXX_INFO(userLogger, m_userName << ": Created");
	updateLastActivity();
}

User::~User(){
	LOG4CXX_INFO(userLogger, m_userName << ": Destroying");

	delete m_rosterManager;
	delete m_conversationManager;
}

void User::setReadyToConnect() {
	if (m_readyForConnect) {
		return;
	}

	LOG4CXX_INFO(userLogger, m_userName << ": Ready to be connected to legacy network");
	m_readyForConnect = true;
	onReadyToConnect();
}

void User::setConnected(bool connected) {
	m_connected = connected;
	m_reconnectCounter = 0;
	setIgnoreDisconnect(false);
	updateLastActivity();

	sendCurrentPresence();

	if (m_connected) {
		// Copy: the handlers can leave rooms.
		std::list<pbnetwork::Room> rooms = m_joinedRooms;
		for (const pbnetwork::Room &room : rooms) {
			doJoinRoom(room);
		}
	}
}

void User::setCacheMessages(bool cacheMessages) {
	if (m_component->inServerMode() && !m_cacheMessages && cacheMessages) {
		m_conversationManager->sendCachedChatMessages();
	}
	m_cacheMessages = cacheMessages;
}

Conversation *User::joinRoom(const std::string &room, const std::string &nickname, const std::string &password) {
	// Joining a room forces the connection to the legacy network, so the backend can handle auto-join.
	setReadyToConnect();

	Conversation *conv = m_conversationManager->getConversation(room);
	if (conv) {
		return conv;
	}

	pbnetwork::Room r;
	r.set_username(m_userName);
	r.set_nickname(nickname);
	r.set_room(room);
	r.set_password(password);

	bool isInJoined = false;
	for (const pbnetwork::Room &joined : m_joinedRooms) {
		if (joined.room() == room) {
			isInJoined = true;
			break;
		}
	}
	if (!isInJoined) {
		m_joinedRooms.push_back(r);
	}

	if (!m_connected) {
		LOG4CXX_INFO(userLogger, m_userName << ": Joining room " << room << " postponed, because use is not connected to legacy network yet.");
		return NULL;
	}

	return doJoinRoom(r);
}

Conversation *User::doJoinRoom(const pbnetwork::Room &room) {
	Conversation *conv = m_conversationManager->getConversation(room.room());
	if (!conv) {
		LOG4CXX_INFO(userLogger, m_userName << ": Going to join room " << room.room() << " as " << room.nickname());
		conv = m_component->getFactory()->createConversation(m_conversationManager, room.room(), true);
		m_conversationManager->addConversation(conv);
		conv->setNickname(room.nickname());
	}

	handleRoomJoined(conv);
	onRoomJoined(room.room(), room.nickname(), room.password());
	return conv;
}

void User::leaveRoom(const std::string &room) {
	onRoomLeft(room);

	for (std::list<pbnetwork::Room>::iterator it = m_joinedRooms.begin(); it != m_joinedRooms.end(); it++) {
		if (it->room() == room) {
			m_joinedRooms.erase(it);
			break;
		}
	}

	Conversation *conv = m_conversationManager->getConversation(room);
	if (conv) {
		m_conversationManager->removeConversation(conv);
		delete conv;
	}
}

void User::setIgnoreDisconnect(bool ignoreDisconnect) {
	m_ignoreDisconnect = ignoreDisconnect;
	LOG4CXX_INFO(userLogger, m_userName << ": Setting ignoreDisconnect=" << m_ignoreDisconnect);
}

void User::handleDisconnected(const std::string &error, pbnetwork::ConnectionError e) {
	if (m_ignoreDisconnect) {
		LOG4CXX_INFO(userLogger, m_userName << ": Disconnecting from legacy network ignored (probably moving between backends)");
		return;
	}

	if (e == pbnetwork::CONNECTION_ERROR_OTHER_ERROR || e == pbnetwork::CONNECTION_ERROR_NETWORK_ERROR) {
		if (m_reconnectLimit < 0 || m_reconnectCounter < m_reconnectLimit) {
			m_reconnectCounter++;
			LOG4CXX_INFO(userLogger, m_userName << ": Disconnecting from legacy network " << error << ", trying to reconnect automatically.");
			// Simulate destruction/resurrection :)
			// TODO: If this stops working, create onReconnect signal
			m_userManager->onUserDestroyed(this);
			m_userManager->onUserCreated(this);
			onReadyToConnect();
			return;
		}
	}

	if (error.empty()) {
		LOG4CXX_INFO(userLogger, m_userName << ": Disconnected from legacy network");
	}
	else {
		LOG4CXX_INFO(userLogger, m_userName << ": Disconnected from legacy network with error " << error);
	}
	onDisconnected();

	disconnectUser(error, e);

	if (m_userManager->getUser(m_userName) != NULL) {
		m_userManager->removeUser(this);
	}
}

}
