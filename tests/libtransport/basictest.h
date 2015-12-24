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

#include <vector>
#include "Swiften/Swiften.h"
#include "Swiften/Queries/SetResponder.h"
#include "transport/Conversation.h"
#include "transport/ConversationManager.h"
#include "transport/UserRegistry.h"
#include "transport/Config.h"
#include "transport/StorageBackend.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/UserManager.h"
#include "transport/UserRegistration.h"
#include "transport/RosterManager.h"
#include "transport/NetworkPluginServer.h"
#include "RosterResponder.h"
#include "discoitemsresponder.h"
#include "transport/LocalBuddy.h"
#include "transport/StorageBackend.h"
#include "transport/Factory.h"
#include "XMPPFrontend.h"
#include "XMPPUserRegistration.h"

#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

using namespace Transport;

class TestingConversation : public Conversation {
	public:
		TestingConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false) : Conversation(conversationManager, legacyName, muc) {
		}

		// Called when there's new message to legacy network from XMPP network
		void sendMessage(boost::shared_ptr<Swift::Message> &message) {
			onMessageToSend(this, message);
		}

		boost::signal<void (TestingConversation *, boost::shared_ptr<Swift::Message> &)> onMessageToSend;
};

class TestingFactory : public Factory {
	public:
		TestingFactory() {
		}

		// Creates new conversation (NetworkConversation in this case)
		Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMuc = false) {
			TestingConversation *nc = new TestingConversation(conversationManager, legacyName, isMuc);
			nc->onMessageToSend.connect(boost::bind(&TestingFactory::handleMessageToSend, this, _1, _2));
			return nc;
		}

		void handleMessageToSend(TestingConversation *_conv, boost::shared_ptr<Swift::Message> &_msg) {
			onMessageToSend(_conv, _msg);
		}

		// Creates new LocalBuddy
		Buddy *createBuddy(RosterManager *rosterManager, const BuddyInfo &buddyInfo) {
			LocalBuddy *buddy = new LocalBuddy(rosterManager, buddyInfo.id, buddyInfo.legacyName, buddyInfo.alias, buddyInfo.groups, (BuddyFlag) buddyInfo.flags);
			if (!buddy->isValid()) {
				delete buddy;
				return NULL;
			}
			buddy->setSubscription(Buddy::Ask);
			if (buddyInfo.settings.find("icon_hash") != buddyInfo.settings.end())
				buddy->setIconHash(buddyInfo.settings.find("icon_hash")->second.s);
			return buddy;
		}

		boost::signal<void (TestingConversation *, boost::shared_ptr<Swift::Message> &)> onMessageToSend;
};

class TestingStorageBackend : public StorageBackend {
	public:
		bool connected;
		std::map<std::string, UserInfo> users;
		std::map<std::string, bool> online_users;
		std::map<int, std::map<std::string, std::string> > settings;
		long buddyid;

		TestingStorageBackend() {
			buddyid = 0;
			connected = false;
		}

		/// connect
		virtual bool connect() {
			connected = true;
			return true;
		}

		/// createDatabase
		virtual bool createDatabase() {return true;}

		/// setUser
		virtual void setUser(const UserInfo &user) {
			users[user.jid] = user;
		}

		/// getuser
		virtual bool getUser(const std::string &barejid, UserInfo &user) {
			if (users.find(barejid) == users.end()) {
				return false;
			}
			user = users[barejid];
			return true;
		}

		std::string findUserByID(long id) {
			for (std::map<std::string, UserInfo>::const_iterator it = users.begin(); it != users.end(); it++) {
				if (it->second.id == id) {
					return it->first;
				}
			}
			return "";
		}

		/// setUserOnline
		virtual void setUserOnline(long id, bool online) {
			std::string user = findUserByID(id);
			if (user.empty()) {
				return;
			}
			online_users[user] = online;
		}

		/// removeUser
		virtual bool removeUser(long id) {
			std::string user = findUserByID(id);
			if (user.empty()) {
				return false;
			}
			users.erase(user);
			return true;
		}

		/// getBuddies
		virtual bool getBuddies(long id, std::list<BuddyInfo> &roster) {
			return true;
		}

		/// getOnlineUsers
		virtual bool getOnlineUsers(std::vector<std::string> &users) {
			return true;
		}

		virtual bool getUsers(std::vector<std::string> &users) {
			return true;
		}

		virtual long addBuddy(long userId, const BuddyInfo &buddyInfo) {
			return buddyid++;
		}
		virtual void updateBuddy(long userId, const BuddyInfo &buddyInfo) {
			
		}

		virtual void removeBuddy(long id) {
			
		}

		virtual void getBuddySetting(long userId, long buddyId, const std::string &variable, int &type, std::string &value) {}
		virtual void updateBuddySetting(long userId, long buddyId, const std::string &variable, int type, const std::string &value) {}

		virtual void getUserSetting(long userId, const std::string &variable, int &type, std::string &value) {
			if (settings[userId].find(variable) == settings[userId].end()) {
				settings[userId][variable] = value;
				return;
			}
			value = settings[userId][variable];
		}

		virtual void updateUserSetting(long userId, const std::string &variable, const std::string &value) {
			settings[userId][variable] = value;
		}

		void dumpUserSettings() {
			std::cout << "\n\nUserSettings dump:\n";
			for (std::map<int, std::map<std::string, std::string> >::const_iterator it = settings.begin(); it != settings.end(); it++) {
				for (std::map<std::string, std::string>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
					std::cout << it->first << ":" << it2->first << "=" << it2->second << "\n";
				}
			}
		}

		virtual void beginTransaction() {}
		virtual void commitTransaction() {}
};

class BasicTest : public Swift::XMPPParserClient {

	public:
		void setMeUp (void);

		void tearMeDown (void);

	void handleDataReceived(const Swift::SafeByteArray &data);
	void handleDataReceived2(const Swift::SafeByteArray &data);

	void handleStreamStart(const Swift::ProtocolHeader&);
#if HAVE_SWIFTEN_3
	void handleElement(boost::shared_ptr<Swift::ToplevelElement> element);
#else
	void handleElement(boost::shared_ptr<Swift::Element> element);
#endif
	void handleStreamEnd();

	void injectPresence(boost::shared_ptr<Swift::Presence> &response);
	void injectIQ(boost::shared_ptr<Swift::IQ> iq);
	void injectMessage(boost::shared_ptr<Swift::Message> msg);

	void dumpReceived();

	void addUser() {
		UserInfo user;
		user.id = 1;
		user.jid = "user@localhost";
		user.uin = "legacyname";
		user.password = "password";
		user.vip = 0;
		storage->setUser(user);
	}

	void connectUser();
	void connectSecondResource();
	void disconnectUser();
	void add2Buddies();

	Swift::Stanza *getStanza(boost::shared_ptr<Swift::Element> element);

	protected:
		bool streamEnded;
		UserManager *userManager;
		boost::shared_ptr<Swift::ServerFromClientSession> serverFromClientSession;
		boost::shared_ptr<Swift::ServerFromClientSession> serverFromClientSession2;
		Swift::FullPayloadSerializerCollection* payloadSerializers;
		Swift::FullPayloadParserFactoryCollection* payloadParserFactories;
		Swift::XMPPParser *parser;
		Swift::XMPPParser *parser2;
		UserRegistry *userRegistry;
		Config *cfg;
		Swift::Server *server;
		Swift::DummyNetworkFactories *factories;
		Swift::DummyEventLoop *loop;
		TestingFactory *factory;
		Component *component;
		std::vector<boost::shared_ptr<Swift::Element> > received;
		std::vector<boost::shared_ptr<Swift::Element> > received2;
		std::string receivedData;
		std::string receivedData2;
		StorageBackend *storage;
		XMPPUserRegistration *userRegistration;
		DiscoItemsResponder *itemsResponder;
		bool stream1_active;
		Transport::XMPPFrontend *frontend;
};

