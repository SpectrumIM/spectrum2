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

#include <string>
#include <algorithm>
#include <Swiften/EventLoop/EventLoop.h>
#include <Swiften/Network/NetworkFactories.h>
#include "Swiften/Elements/RosterPayload.h"
#include "Swiften/Elements/VCard.h"
#include "Swiften/Elements/Message.h"
#include "Swiften/Elements/IQ.h"
#include "Swiften/Elements/DiscoInfo.h"
#include "Swiften/Elements/Presence.h"

#include <boost/signal.hpp>

namespace Transport {

class Config;
class UserRegistry;
class Component;
class User;
class Buddy;
class RosterManager;
class UserManager;
class StorageBackend;
class Frontend;

struct UserInfo;

class FrontendPlugin {
	public:
		virtual std::string name() const = 0;
		virtual Frontend *createFrontend() = 0;

		virtual ~FrontendPlugin() {};
};


class Frontend {
	public:
		Frontend() {}

		virtual ~Frontend() {}

		virtual void init(Component *component, Swift::EventLoop *loop, Swift::NetworkFactories *factories, Config *config, UserRegistry *userRegistry) = 0;

		virtual void connectToServer() = 0;

		virtual void disconnectFromServer() = 0;

		virtual void sendPresence(Swift::Presence::ref presence) = 0;

		virtual void sendVCard(Swift::VCard::ref vcard, Swift::JID to) = 0;

		virtual void sendRosterRequest(Swift::RosterPayload::ref, Swift::JID to) = 0;

		virtual void sendMessage(boost::shared_ptr<Swift::Message> message) = 0;

		virtual void sendIQ(boost::shared_ptr<Swift::IQ>) = 0;

		virtual boost::shared_ptr<Swift::DiscoInfo> sendCapabilitiesRequest(Swift::JID to) = 0;

		virtual void reconnectUser(const std::string &user) = 0;

		virtual RosterManager *createRosterManager(User *user, Component *component) = 0;

		virtual User *createUser(const Swift::JID &jid, UserInfo &userInfo, Component *component, UserManager *userManager) = 0;

		virtual UserManager *createUserManager(Component *component, UserRegistry *userRegistry, StorageBackend *storageBackend = NULL) = 0;

		virtual void clearRoomList() = 0;
		virtual void addRoomToRoomList(const std::string &handle, const std::string &name) = 0;

		virtual std::string setOAuth2Code(const std::string &code, const std::string &state) { return "OAuth2 code is not needed for this frontend."; }
		virtual std::string getOAuth2URL(const std::vector<std::string> &args) { return ""; }
		virtual std::string getRegistrationFields() { return "Jabber ID\n3rd-party network username\n3rd-party network password"; }

		virtual bool isRawXMLEnabled() { return false; }

		boost::signal<void (User *, const std::string &name, unsigned int id)> onVCardRequired;
		boost::signal<void (User *, boost::shared_ptr<Swift::VCard> vcard)> onVCardUpdated;
		boost::signal<void (Buddy *, const Swift::RosterItemPayload &item)> onBuddyUpdated;
		boost::signal<void (Buddy *)> onBuddyRemoved;
		boost::signal<void (Buddy *, const Swift::RosterItemPayload &item)> onBuddyAdded;
		boost::signal<void (Swift::Message::ref message)> onMessageReceived;
		boost::signal<void (bool /* isAvailable */)> onAvailableChanged;
		boost::signal<void (boost::shared_ptr<Swift::Presence>) > onPresenceReceived;
		boost::signal<void (const Swift::JID& jid, boost::shared_ptr<Swift::DiscoInfo> info)> onCapabilitiesReceived;
};

}
