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
#include "SlackUserRegistration.h"
#include "SlackFrontend.h"

#include <Swiften/Swiften.h>
#include <Swiften/EventLoop/DummyEventLoop.h>
#include <Swiften/Server/Server.h>
#include <Swiften/Network/DummyNetworkFactories.h>
#include <Swiften/Network/DummyConnectionServer.h>
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Server/ServerFromClientSession.h"
#include "Swiften/Parser/PayloadParsers/FullPayloadParserFactoryCollection.h"
#define HAVE_SWIFTEN_3  (SWIFTEN_VERSION >= 0x030000)

#include "basictest.h"

using namespace Transport;

class BasicSlackTest {

	public:
		void setMeUp (void);

		void tearMeDown (void);

	void addUser() {
		UserInfo user;
		user.id = 1;
		user.jid = "user@localhost";
		user.uin = "legacyname";
		user.password = "password";
		user.vip = 0;
		storage->setUser(user);
	}

	protected:
		UserManager *userManager;
		UserRegistry *userRegistry;
		Config *cfg;
		Swift::Server *server;
		Swift::DummyNetworkFactories *factories;
		Swift::DummyEventLoop *loop;
		TestingFactory *factory;
		Component *component;
		StorageBackend *storage;
		SlackUserRegistration *userRegistration;
		Transport::SlackFrontend *frontend;
};

