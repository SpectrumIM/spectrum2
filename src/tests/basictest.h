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
#include "transport/conversation.h"
#include "transport/conversationmanager.h"
#include "transport/userregistry.h"
#include "transport/config.h"
#include "transport/storagebackend.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/conversation.h"
#include "transport/usermanager.h"
#include "transport/localbuddy.h"

#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"

using namespace Transport;

class TestingConversation : public Conversation {
	public:
		TestingConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false) : Conversation(conversationManager, legacyName, muc) {
		}

		// Called when there's new message to legacy network from XMPP network
		void sendMessage(boost::shared_ptr<Swift::Message> &message) {

		}
};

class TestingFactory : public Factory {
	public:
		TestingFactory() {
		}

		// Creates new conversation (NetworkConversation in this case)
		Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName) {
			Conversation *nc = new TestingConversation(conversationManager, legacyName);
			return nc;
		}

		// Creates new LocalBuddy
		Buddy *createBuddy(RosterManager *rosterManager, const BuddyInfo &buddyInfo) {
			LocalBuddy *buddy = new LocalBuddy(rosterManager, buddyInfo.id);
			buddy->setAlias(buddyInfo.alias);
			buddy->setName(buddyInfo.legacyName);
			buddy->setSubscription(Buddy::Ask);
			buddy->setGroups(buddyInfo.groups);
			buddy->setFlags((BuddyFlag) buddyInfo.flags);
			if (buddyInfo.settings.find("icon_hash") != buddyInfo.settings.end())
				buddy->setIconHash(buddyInfo.settings.find("icon_hash")->second.s);
			return buddy;
		}
};

class BasicTest : public Swift::XMPPParserClient {

	public:
		void setMeUp (void);

		void tearMeDown (void);

	void handleDataReceived(const Swift::SafeByteArray &data);

	void handleStreamStart(const Swift::ProtocolHeader&);

	void handleElement(boost::shared_ptr<Swift::Element> element);

	void handleStreamEnd();

	void injectPresence(boost::shared_ptr<Swift::Presence> &response);
	void injectIQ(boost::shared_ptr<Swift::IQ> iq);

	Swift::Stanza *getStanza(boost::shared_ptr<Swift::Element> element);

	protected:
		bool streamEnded;
		UserManager *userManager;
		boost::shared_ptr<Swift::ServerFromClientSession> serverFromClientSession;
		Swift::FullPayloadSerializerCollection* payloadSerializers;
		Swift::FullPayloadParserFactoryCollection* payloadParserFactories;
		Swift::XMPPParser *parser;
		UserRegistry *userRegistry;
		Config *cfg;
		Swift::Server *server;
		Swift::DummyNetworkFactories *factories;
		Swift::DummyEventLoop *loop;
		TestingFactory *factory;
		Component *component;
		std::vector<boost::shared_ptr<Swift::Element> > received;
};

