/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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
#include "transport/protocol.pb.h"

#include <string>
#include <vector>

namespace Transport {

class Config;
class UserRegistry;
class Component;
class User;
class Buddy;
class RosterManager;
class UserManager;
class StorageBackend;
class Conversation;
class ConversationManager;
class Frontend;

struct UserInfo;

class FrontendPlugin {
	public:
		virtual std::string name() const = 0;
		virtual Frontend *createFrontend() = 0;

		virtual ~FrontendPlugin() {};
};

/// Connects the core to the users' side (XMPP, Slack, ...).
///
/// The core speaks protobuf (protocol.proto) and plain strings only. A user is
/// identified by a string (the bare JID for XMPP) and buddies and rooms by their
/// legacy network names. The frontend translates its own addressing to and from
/// these. Per-user behaviour lives in the frontend's subclasses of User,
/// RosterManager, UserManager and Conversation, which the frontend creates.
class Frontend {
	public:
		Frontend() {}

		virtual ~Frontend() {}

		virtual void init(Component *component, Config *config, UserRegistry *userRegistry) = 0;

		virtual void connectToServer() = 0;

		virtual void disconnectFromServer() = 0;

		virtual void reconnectUser(const std::string &user) = 0;

		virtual RosterManager *createRosterManager(User *user, Component *component) = 0;

		virtual User *createUser(const std::string &user, UserInfo &userInfo, Component *component, UserManager *userManager) = 0;

		virtual UserManager *createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL) = 0;

		virtual Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMUC = false) = 0;

		/// The backend finished the login of user: connected, or rejected it with error.
		virtual void handleLoginResult(const std::string &user, bool connected, const std::string &error) {}

		/// Raw XML the backend sent (features.rawxml).
		virtual void sendRawXML(const std::string &xml) {}

		virtual void clearRoomList() = 0;
		virtual void addRoomToRoomList(const std::string &room, const std::string &name) = 0;

		virtual std::string setOAuth2Code(const std::string &code, const std::string &state) { return "OAuth2 code is not needed for this frontend."; }
		virtual std::string getOAuth2URL(const std::vector<std::string> &args) { return ""; }
		virtual std::string getRegistrationFields() { return "Jabber ID\n3rd-party network username\n3rd-party network password"; }

		virtual bool isRawXMLEnabled() { return false; }

		Event<void (User *, const std::string &name, unsigned int id)> onVCardRequired;
		Event<void (User *, const pbnetwork::VCard &vcard)> onVCardUpdated;
		Event<void (Buddy *, const pbnetwork::Buddy &buddy)> onBuddyUpdated;
		Event<void (Buddy *)> onBuddyRemoved;
		Event<void (Buddy *, const pbnetwork::Buddy &buddy)> onBuddyAdded;
		/// Raw XML from the user for the backend (features.rawxml).
		Event<void (User *, const std::string &xml)> onRawXMLReceived;
};

}
