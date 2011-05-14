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

#include "transport/networkpluginserver.h"
#include "transport/user.h"
#include "transport/transport.h"
#include "transport/storagebackend.h"
#include "transport/rostermanager.h"
#include "transport/usermanager.h"
#include "transport/conversationmanager.h"
#include "transport/localbuddy.h"
#include "transport/config.h"
#include "transport/conversation.h"
#include "Swiften/Swiften.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "pbnetwork.pb.h"
#include "sys/wait.h"
#include "sys/signal.h"

namespace Transport {

class NetworkConversation : public Conversation {
	public:
		NetworkConversation(ConversationManager *conversationManager, const std::string &legacyName) : Conversation(conversationManager, legacyName) {
		}

		void sendMessage(boost::shared_ptr<Swift::Message> &message) {
			onMessageToSend(this, message);
		}

		boost::signal<void (NetworkConversation *, boost::shared_ptr<Swift::Message> &)> onMessageToSend;
};

class NetworkFactory : public Factory {
	public:
		NetworkFactory(NetworkPluginServer *nps) {
			m_nps = nps;
		}
		
		Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName) {
			NetworkConversation *nc = new NetworkConversation(conversationManager, legacyName);
			nc->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, m_nps, _1, _2));
			return nc;
		}

		Buddy *createBuddy(RosterManager *rosterManager, const BuddyInfo &buddyInfo) {
			return new LocalBuddy(rosterManager, -1);
		}
	private:
		NetworkPluginServer *m_nps;
};

#define WRAP(MESSAGE, TYPE) 	pbnetwork::WrapperMessage wrap; \
	wrap.set_type(TYPE); \
	wrap.set_payload(MESSAGE); \
	wrap.SerializeToString(&MESSAGE);
	
static int exec_(const char *path, const char *host, const char *port, const char *config) {
// 	char *argv[] = {(char*)script_name, '\0'}; 
	int status = 0;
	pid_t pid = fork();
	if ( pid == 0 ) {
		// child process
		execlp(path, path, "--host", host, "--port", port, config, NULL);
		exit(1);
	} else if ( pid < 0 ) {
		// fork failed
		status = -1;
	}
	return status;
}

static void SigCatcher(int n) {
	wait3(NULL,WNOHANG,NULL);
}

static void handleBuddyPayload(LocalBuddy *buddy, const pbnetwork::Buddy &payload) {
	buddy->setName(payload.buddyname());
	buddy->setAlias(payload.alias());
	std::vector<std::string> groups;
	groups.push_back(payload.groups());
	buddy->setGroups(groups);
	buddy->setStatus(Swift::StatusShow((Swift::StatusShow::Type) payload.status()), payload.statusmessage());
	buddy->setIconHash(payload.iconhash());
}

NetworkPluginServer::NetworkPluginServer(Component *component, Config *config, UserManager *userManager) {
	m_userManager = userManager;
	m_config = config;
	m_pongReceived = false;
	m_userManager->onUserCreated.connect(boost::bind(&NetworkPluginServer::handleUserCreated, this, _1));
	m_userManager->onUserDestroyed.connect(boost::bind(&NetworkPluginServer::handleUserDestroyed, this, _1));

	m_pingTimer = component->getFactories()->getTimerFactory()->createTimer(10000);
	m_pingTimer->onTick.connect(boost::bind(&NetworkPluginServer::pingTimeout, this)); 

	m_server = component->getFactories()->getConnectionFactory()->createConnectionServer(10000);
	m_server->onNewConnection.connect(boost::bind(&NetworkPluginServer::handleNewClientConnection, this, _1));
	m_server->start();

	signal(SIGCHLD, SigCatcher);

	exec_(CONFIG_STRING(m_config, "service.backend").c_str(), "localhost", "10000", m_config->getConfigFile().c_str());
}

NetworkPluginServer::~NetworkPluginServer() {
	m_pingTimer->stop();
}

void NetworkPluginServer::handleNewClientConnection(boost::shared_ptr<Swift::Connection> c) {
	if (m_client) {
		c->disconnect();
	}
	m_client = c;
	m_pongReceived = false;
	

	c->onDisconnected.connect(boost::bind(&NetworkPluginServer::handleSessionFinished, this, c));
	c->onDataRead.connect(boost::bind(&NetworkPluginServer::handleDataRead, this, c, _1));
	sendPing();
	m_pingTimer->start();
}

void NetworkPluginServer::handleSessionFinished(boost::shared_ptr<Swift::Connection> c) {
	if (c == m_client) {
		m_client.reset();
	}
	m_pingTimer->stop();
	exec_(CONFIG_STRING(m_config, "service.backend").c_str(), "localhost", "10000", m_config->getConfigFile().c_str());
}

void NetworkPluginServer::handleConnectedPayload(const std::string &data) {
	pbnetwork::Connected payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	std::cout << payload.name() << "\n";
}

void NetworkPluginServer::handleDisconnectedPayload(const std::string &data) {
	pbnetwork::Disconnected payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.user());
	if (!user)
		return;

	user->handleDisconnected(payload.message());
}

void NetworkPluginServer::handleBuddyChangedPayload(const std::string &data) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	LocalBuddy *buddy = (LocalBuddy *) user->getRosterManager()->getBuddy(payload.buddyname());
	if (buddy) {
		handleBuddyPayload(buddy, payload);
		buddy->buddyChanged();
	}
	else {
		buddy = new LocalBuddy(user->getRosterManager(), -1);
		handleBuddyPayload(buddy, payload);
		user->getRosterManager()->setBuddy(buddy);
	}
}

void NetworkPluginServer::handleConvMessagePayload(const std::string &data) {
	pbnetwork::ConversationMessage payload;
	std::cout << "payload...\n";
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	std::cout << "payload 2...\n";
	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
	}
	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(payload.message());
	conv->handleMessage(msg);
}

void NetworkPluginServer::handleDataRead(boost::shared_ptr<Swift::Connection> c, const Swift::ByteArray &data) {
	long expected_size = 0;
	m_data += data.toString();
	std::cout << "received data; size = " << m_data.size() << "\n";
	while (m_data.size() != 0) {
		if (m_data.size() >= 4) {
			unsigned char * head = (unsigned char*) m_data.c_str();
			expected_size = (((((*head << 8) | *(head + 1)) << 8) | *(head + 2)) << 8) | *(head + 3);
			//expected_size = m_data[0];
			std::cout << "expected_size=" << expected_size << "\n";
			if (m_data.size() - 4 < expected_size)
				return;
		}
		else {
			return;
		}

		std::string msg = m_data.substr(4, expected_size);
		m_data.erase(0, 4 + expected_size);

		pbnetwork::WrapperMessage wrapper;
		if (wrapper.ParseFromString(msg) == false) {
			// TODO: ERROR
			return;
		}

		switch(wrapper.type()) {
			case pbnetwork::WrapperMessage_Type_TYPE_CONNECTED:
				handleConnectedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_DISCONNECTED:
				handleDisconnectedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED:
				handleBuddyChangedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE:
				handleConvMessagePayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_PONG:
				m_pongReceived = true;
				break;
			default:
				return;
		}
	}
}

void NetworkPluginServer::send(boost::shared_ptr<Swift::Connection> &c, const std::string &data) {
	std::string header("    ");
	for (int i = 0; i != 4; ++i)
		header.at(i) = static_cast<char>(data.size() >> (8 * (3 - i)));
	
	c->write(Swift::ByteArray(header + data));
}

void NetworkPluginServer::pingTimeout() {
	std::cout << "pingtimeout\n";
	if (m_pongReceived) {
		sendPing();
		m_pingTimer->start();
	}
	else {
		exec_(CONFIG_STRING(m_config, "service.backend").c_str(), "localhost", "10000", m_config->getConfigFile().c_str());
	}
}

void NetworkPluginServer::handleUserCreated(User *user) {
// 	UserInfo userInfo = user->getUserInfo();
	user->onReadyToConnect.connect(boost::bind(&NetworkPluginServer::handleUserReadyToConnect, this, user));
}

void NetworkPluginServer::handleUserReadyToConnect(User *user) {
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Login login;
	login.set_user(user->getJID().toBare());
	login.set_legacyname(userInfo.uin);
	login.set_password(userInfo.password);

	std::string message;
	login.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_LOGIN);

	send(m_client, message);
}

void NetworkPluginServer::handleUserDestroyed(User *user) {
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Logout logout;
	logout.set_user(user->getJID().toBare());
	logout.set_legacyname(userInfo.uin);

	std::string message;
	logout.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_LOGOUT);
 
	send(m_client, message);
}

void NetworkPluginServer::handleMessageReceived(NetworkConversation *conv, boost::shared_ptr<Swift::Message> &message) {
	
}

void NetworkPluginServer::sendPing() {

	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_PING);
	wrap.SerializeToString(&message);

	send(m_client, message);
	m_pongReceived = false;
	std::cout << "SENDING PING\n";
}

}
