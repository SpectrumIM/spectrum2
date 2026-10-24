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

#include "transport/Event.h"

#include <time.h>

#include <functional>
#include <list>
#include <string>

#include "transport/StorageBackend.h"
#include "transport/protocol.pb.h"

namespace Transport {

class Component;
class RosterManager;
class Conversation;
class ConversationManager;
class UserManager;

/// Represents online user.
class User {
	public:
		/// Creates new User class.
		/// \param user user name (the bare JID for XMPP) associated with this user
		/// \param userInfo UserInfo struct with informations needed to connect
		/// this user to legacy network
		/// \param component Component associated with this user
		User(const std::string &user, UserInfo &userInfo, Component * component, UserManager *userManager);

		/// Destroyes User.
		virtual ~User();

		/// Returns name of the user (the bare JID for XMPP).
		const std::string &getUserName() { return m_userName; }

		/// Returns UserInfo struct with informations needed to connect the legacy network.
		/// \return UserInfo struct
		UserInfo &getUserInfo() { return m_userInfo; }

		RosterManager *getRosterManager() { return m_rosterManager; }

		ConversationManager *getConversationManager() { return m_conversationManager; }

		Component *getComponent() { return m_component; }

		UserManager *getUserManager() { return m_userManager; }

		virtual void disconnectUser(const std::string &error, pbnetwork::ConnectionError e) = 0;
		virtual void clearRoomList() {}
		virtual void addRoomToRoomList(const std::string &room, const std::string &name) {}
		virtual void requestVCard() {}

		/// Sends the user's current state (transport contact status) to the user's clients.
		virtual void sendCurrentPresence() {}

		void setData(void *data) { m_data = data; }
		void *getData() { return m_data; }

		time_t &getLastActivity() {
			return m_lastActivity;
		}

		void updateLastActivity() {
			m_lastActivity = time(NULL);
		}

		/// Returns language.
		/// \return language
		const char *getLang() { return "en"; }

		void handleDisconnected(const std::string &error, pbnetwork::ConnectionError e = pbnetwork::CONNECTION_ERROR_OTHER_ERROR);

		bool isReadyToConnect() {
			return m_readyForConnect;
		}

		/// Marks the user as ready to be connected to the legacy network (once).
		void setReadyToConnect();

		void setConnected(bool connected);

		void setIgnoreDisconnect(bool ignoreDisconnect);

		bool isConnected() {
			return m_connected;
		}

		void addUserSetting(const std::string &key, const std::string &value) {
			m_userInfo.settings[key] = value;
		}

		const std::string &getUserSetting(const std::string &key) {
			return m_userInfo.settings[key];
		}

		void setCacheMessages(bool cacheMessages);

		bool shouldCacheMessages() {
			return m_cacheMessages;
		}

		void setReconnectLimit(int limit) {
			m_reconnectLimit = limit;
		}

		void setStorageBackend(StorageBackend *storageBackend) {
			m_storageBackend = storageBackend;
		}

		StorageBackend *getStorageBackend() {
			return m_storageBackend;
		}

		/// Joins the room on the legacy network. Postponed until the user is connected.
		/// \return the room's Conversation, or NULL if the join is postponed.
		Conversation *joinRoom(const std::string &room, const std::string &nickname, const std::string &password);

		virtual void leaveRoom(const std::string &room);

		Event<void ()> onReadyToConnect;
		/// The user's own status changed (STATUS_NONE: all the user's clients are gone).
		Event<void (const pbnetwork::Status &status)> onStatusChanged;
		Event<void (const std::string &room, const std::string &nickname, const std::string &password)> onRoomJoined;
		Event<void (const std::string &room)> onRoomLeft;
		Event<void ()> onDisconnected;

	protected:
		/// Called whenever the user (re)joins a room on the legacy network, right before onRoomJoined.
		virtual void handleRoomJoined(Conversation *conv) {}

	private:
		Conversation *doJoinRoom(const pbnetwork::Room &room);

		std::string m_userName;
		Component *m_component;
		RosterManager *m_rosterManager;
		UserManager *m_userManager;
		ConversationManager *m_conversationManager;
		UserInfo m_userInfo;
		void *m_data;
		bool m_connected;
		bool m_readyForConnect;
		bool m_ignoreDisconnect;
		time_t m_lastActivity;
		int m_reconnectCounter;
		std::list<pbnetwork::Room> m_joinedRooms;
		bool m_cacheMessages;
		int m_reconnectLimit;
		StorageBackend *m_storageBackend;
};

}
