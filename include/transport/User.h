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

#include <boost/signal.hpp>
#include <time.h>
#include "transport/StorageBackend.h"
#include <Swiften/FileTransfer/OutgoingFileTransfer.h>
#include "Swiften/Elements/SpectrumErrorPayload.h"
#include "Swiften/JID/JID.h"
#include "Swiften/Elements/Presence.h"
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Network/Timer.h"
#include "Swiften/Network/Connection.h"

namespace Transport {

class Component;
class RosterManager;
class ConversationManager;
class UserManager;
class PresenceOracle;

/// Represents online XMPP user.
class User {
	public:
		/// Creates new User class.
		/// \param jid XMPP JID associated with this user
		/// \param userInfo UserInfo struct with informations needed to connect
		/// this user to legacy network
		/// \param component Component associated with this user
		User(const Swift::JID &jid, UserInfo &userInfo, Component * component, UserManager *userManager);

		/// Destroyes User.
		virtual ~User();

		/// Returns JID of XMPP user who is currently connected using this User class.
		/// \return full JID
		const Swift::JID &getJID();

		/// Returns full JID which supports particular feature or invalid JID.
		/// \param feature disco#info feature.
		/// \return full JID which supports particular feature or invalid JID.
		std::vector<Swift::JID> getJIDWithFeature(const std::string &feature);

// 		Swift::DiscoInfo::ref getCaps(const Swift::JID &jid) const;

		/// Returns UserInfo struct with informations needed to connect the legacy network.
		/// \return UserInfo struct
		UserInfo &getUserInfo() { return m_userInfo; }

		RosterManager *getRosterManager() { return m_rosterManager; }

		ConversationManager *getConversationManager() { return m_conversationManager; }

		Component *getComponent() { return m_component; }

		UserManager *getUserManager() { return m_userManager; }
		
		virtual void disconnectUser(const std::string &error, Swift::SpectrumErrorPayload::Error e) = 0;
		virtual void clearRoomList() {}
		virtual void addRoomToRoomList(const std::string &handle, const std::string &name) {}
		virtual void requestVCard() {}

		void setData(void *data) { m_data = data; }
		void *getData() { return m_data; }

		/// Handles presence from XMPP JID associated with this user.
		/// \param presence Swift::Presence.
		void handlePresence(Swift::Presence::ref presence, bool forceJoin = false);

		void handleSubscription(Swift::Presence::ref presence);

		void handleDiscoInfo(const Swift::JID& jid, std::shared_ptr<Swift::DiscoInfo> info);

		time_t &getLastActivity() {
			return m_lastActivity;
		}

		void updateLastActivity() {
			m_lastActivity = time(NULL);
		}

		/// Returns language.
		/// \return language
		const char *getLang() { return "en"; }

		void handleDisconnected(const std::string &error, Swift::SpectrumErrorPayload::Error e = Swift::SpectrumErrorPayload::CONNECTION_ERROR_OTHER_ERROR);

		bool isReadyToConnect() {
			return m_readyForConnect;
		}

		void setConnected(bool connected);

		void sendCurrentPresence();

		void setIgnoreDisconnect(bool ignoreDisconnect);

		bool isConnected() {
			return m_connected;
		}

		int getResourceCount() {
			return m_resources;
		}

		void addUserSetting(const std::string &key, const std::string &value) {
			m_settings[key] = value;
		}

		const std::string &getUserSetting(const std::string &key) {
			return m_settings[key];
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

		void leaveRoom(const std::string &room);

		boost::signal<void ()> onReadyToConnect;
		boost::signal<void (Swift::Presence::ref presence)> onPresenceChanged;
		boost::signal<void (Swift::Presence::ref presence)> onRawPresenceReceived;
		boost::signal<void (const Swift::JID &who, const std::string &room, const std::string &nickname, const std::string &password)> onRoomJoined;
		boost::signal<void (const std::string &room)> onRoomLeft;
		boost::signal<void ()> onDisconnected;

	private:
		void onConnectingTimeout();

		Swift::JID m_jid;
		Component *m_component;
		RosterManager *m_rosterManager;
		UserManager *m_userManager;
		ConversationManager *m_conversationManager;
		PresenceOracle *m_presenceOracle;
		UserInfo m_userInfo;
		void *m_data;
		bool m_connected;
		bool m_readyForConnect;
		bool m_ignoreDisconnect;
		Swift::Timer::ref m_reconnectTimer;
		std::shared_ptr<Swift::Connection> connection;
		time_t m_lastActivity;
		std::map<Swift::JID, Swift::DiscoInfo::ref> m_legacyCaps;
		std::vector<std::shared_ptr<Swift::OutgoingFileTransfer> > m_filetransfers;
		int m_resources;
		int m_reconnectCounter;
		std::list<Swift::Presence::ref> m_joinedRooms;
		std::map<std::string, std::string> m_settings;
		bool m_cacheMessages;
		int m_reconnectLimit;
		StorageBackend *m_storageBackend;
};

}
