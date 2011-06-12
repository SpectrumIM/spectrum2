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
#include "transport/vcardresponder.h"
#include "transport/rosterresponder.h"
#include "Swiften/Swiften.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "pbnetwork.pb.h"
#include "sys/wait.h"
#include "sys/signal.h"

namespace Transport {

class NetworkConversation : public Conversation {
	public:
		NetworkConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false) : Conversation(conversationManager, legacyName, muc) {
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
			LocalBuddy *buddy = new LocalBuddy(rosterManager, buddyInfo.id);
			buddy->setAlias(buddyInfo.alias);
			buddy->setName(buddyInfo.legacyName);
			buddy->setSubscription(buddyInfo.subscription);
			buddy->setGroups(buddyInfo.groups);
			buddy->setFlags((BuddyFlag) buddyInfo.flags);
			if (buddyInfo.settings.find("icon_hash") != buddyInfo.settings.end())
				buddy->setIconHash(buddyInfo.settings.find("icon_hash")->second.s);
			return buddy;
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
		abort();
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
	m_component = component;
	m_component->m_factory = new NetworkFactory(this);
	m_userManager->onUserCreated.connect(boost::bind(&NetworkPluginServer::handleUserCreated, this, _1));
	m_userManager->onUserDestroyed.connect(boost::bind(&NetworkPluginServer::handleUserDestroyed, this, _1));

	m_pingTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(10000);
	m_pingTimer->onTick.connect(boost::bind(&NetworkPluginServer::pingTimeout, this)); 

	m_vcardResponder = new VCardResponder(component->getIQRouter(), userManager);
	m_vcardResponder->onVCardRequired.connect(boost::bind(&NetworkPluginServer::handleVCardRequired, this, _1, _2, _3));
	m_vcardResponder->start();

	m_rosterResponder = new RosterResponder(component->getIQRouter(), userManager);
	m_rosterResponder->onBuddyAdded.connect(boost::bind(&NetworkPluginServer::handleBuddyAdded, this, _1, _2));
	m_rosterResponder->onBuddyRemoved.connect(boost::bind(&NetworkPluginServer::handleBuddyRemoved, this, _1));
	m_rosterResponder->onBuddyUpdated.connect(boost::bind(&NetworkPluginServer::handleBuddyUpdated, this, _1, _2));
	m_rosterResponder->start();

	m_server = component->getNetworkFactories()->getConnectionFactory()->createConnectionServer(boost::lexical_cast<int>(CONFIG_STRING(m_config, "service.backend_port")));
	m_server->onNewConnection.connect(boost::bind(&NetworkPluginServer::handleNewClientConnection, this, _1));
	m_server->start();

	signal(SIGCHLD, SigCatcher);

	exec_(CONFIG_STRING(m_config, "service.backend").c_str(), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
}

NetworkPluginServer::~NetworkPluginServer() {
	m_pingTimer->stop();
	delete m_vcardResponder;
	delete m_rosterResponder;
}

void NetworkPluginServer::handleNewClientConnection(boost::shared_ptr<Swift::Connection> c) {
	Client *client = new Client;
	client->pongReceived = true;
	client->connection = c;

	m_clients.push_back(client);

	c->onDisconnected.connect(boost::bind(&NetworkPluginServer::handleSessionFinished, this, client));
	c->onDataRead.connect(boost::bind(&NetworkPluginServer::handleDataRead, this, client, _1));
	sendPing(client);
	m_pingTimer->start();
}

void NetworkPluginServer::handleSessionFinished(Client *c) {
	for (std::list<User *>::const_iterator it = c->users.begin(); it != c->users.end(); it++) {
		(*it)->setData(NULL);
		(*it)->handleDisconnected("Internal Server Error, please reconnect.");
	}

	m_clients.remove(c);
	delete c;

	// Execute new session only if there's no free one after this crash/disconnection
	for (std::list<Client *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->users.size() < CONFIG_INT(m_config, "service.users_per_backend")) {
			return;
		}
	}
	exec_(CONFIG_STRING(m_config, "service.backend").c_str(), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
}

void NetworkPluginServer::handleConnectedPayload(const std::string &data) {
	pbnetwork::Connected payload;
	std::cout << "CONNECTED LOGIN 2 " << payload.user() << "\n";
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	std::cout << "CONNECTED LOGIN 3 " << payload.user() << "\n";
	m_component->m_userRegistry->onPasswordValid(payload.user());
// 	std::cout << payload.name() << "\n";
}

void NetworkPluginServer::handleDisconnectedPayload(const std::string &data) {
	pbnetwork::Disconnected payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	m_component->m_userRegistry->onPasswordInvalid(payload.user());

	User *user = m_userManager->getUser(payload.user());
	if (!user) {
		return;
	}
	user->handleDisconnected(payload.message());
}

void NetworkPluginServer::handleVCardPayload(const std::string &data) {
	pbnetwork::VCard payload;
	if (payload.ParseFromString(data) == false) {
		std::cout << "PARSING ERROR\n";
		// TODO: ERROR
		return;
	}
	std::cout << "OMG?\n";
	boost::shared_ptr<Swift::VCard> vcard(new Swift::VCard());
	vcard->setFullName(payload.fullname());
	vcard->setPhoto(Swift::createByteArray(payload.photo()));
	vcard->setNickname(payload.nickname());

	m_vcardResponder->sendVCard(payload.id(), vcard);
}

void NetworkPluginServer::handleChatStatePayload(const std::string &data, Swift::ChatState::ChatStateType type) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		std::cout << "handling chatstate: NO conv with buddyname=" << payload.buddyname() << "\n";
		return;
	}

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->addPayload(boost::make_shared<Swift::ChatState>(type));

	conv->handleMessage(msg);
	
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
		buddy->handleBuddyChanged();
	}
	else {
		buddy = new LocalBuddy(user->getRosterManager(), -1);
		handleBuddyPayload(buddy, payload);
		user->getRosterManager()->setBuddy(buddy);
	}
}

void NetworkPluginServer::handleParticipantChangedPayload(const std::string &data) {
	pbnetwork::Participant payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.room());
	if (!conv) {
		return;
	}

	conv->handleParticipantChanged(payload.nickname(), payload.flag(), payload.status(), payload.statusmessage(), payload.newname());

// 	LocalBuddy *buddy = (LocalBuddy *) user->getRosterManager()->getBuddy(payload.buddyname());
// 	if (buddy) {
// 		handleBuddyPayload(buddy, payload);
// 		buddy->buddyChanged();
// 	}
// 	else {
// 		buddy = new LocalBuddy(user->getRosterManager(), -1);
// 		handleBuddyPayload(buddy, payload);
// 		user->getRosterManager()->setBuddy(buddy);
// 	}
// 	std::cout << payload.nickname() << "\n";
}

void NetworkPluginServer::handleRoomChangedPayload(const std::string &data) {
	pbnetwork::Room payload;
	if (payload.ParseFromString(data) == false) {
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.room());
	if (!conv) {
		return;
	}

	conv->setNickname(payload.nickname());
}

void NetworkPluginServer::handleConvMessagePayload(const std::string &data, bool subject) {
	pbnetwork::ConversationMessage payload;
// 	std::cout << "payload...\n";
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
// 	std::cout << "payload 2...\n";
	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	if (subject) {
		msg->setSubject(payload.message());
	}
	else {
		msg->setBody(payload.message());
	}

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
		conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
	}

	conv->handleMessage(msg, payload.nickname());
}

void NetworkPluginServer::handleDataRead(Client *c, const Swift::SafeByteArray &data) {
	c->data.insert(c->data.end(), data.begin(), data.end());
	while (c->data.size() != 0) {
		unsigned int expected_size;

		if (c->data.size() >= 4) {
			expected_size = *((unsigned int*) &c->data[0]);
			expected_size = ntohl(expected_size);
			if (c->data.size() - 4 < expected_size)
				return;
		}
		else {
			return;
		}

		pbnetwork::WrapperMessage wrapper;
		if (wrapper.ParseFromArray(&c->data[4], expected_size) == false) {
			std::cout << "PARSING ERROR " << expected_size << "\n";
			c->data.erase(c->data.begin(), c->data.begin() + 4 + expected_size);
			continue;
		}
		c->data.erase(c->data.begin(), c->data.begin() + 4 + expected_size);

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
			case pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED:
				handleConvMessagePayload(wrapper.payload(), true);
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_PONG:
				c->pongReceived = true;
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_PARTICIPANT_CHANGED:
				handleParticipantChangedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_ROOM_NICKNAME_CHANGED:
				handleRoomChangedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_VCARD:
				handleVCardPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING:
				handleChatStatePayload(wrapper.payload(), Swift::ChatState::Composing);
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED:
				handleChatStatePayload(wrapper.payload(), Swift::ChatState::Paused);
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING:
				handleChatStatePayload(wrapper.payload(), Swift::ChatState::Active);
				break;
			default:
				return;
		}
	}
}

void NetworkPluginServer::send(boost::shared_ptr<Swift::Connection> &c, const std::string &data) {
	char header[4];
	*((int*)(header)) = htonl(data.size());
	c->write(Swift::createSafeByteArray(std::string(header, 4) + data));
}

void NetworkPluginServer::pingTimeout() {
	std::cout << "pingtimeout\n";
	for (std::list<Client *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->pongReceived) {
			sendPing((*it));
			m_pingTimer->start();
		}
		else {
			exec_(CONFIG_STRING(m_config, "service.backend").c_str(), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
		}
	}
}

void NetworkPluginServer::handleUserCreated(User *user) {
	Client *c = getFreeClient();
	if (!c) {
		user->handleDisconnected("Internal Server Error, please reconnect.");
		return;
	}
	user->setData(c);
	c->users.push_back(user);

// 	UserInfo userInfo = user->getUserInfo();
	user->onReadyToConnect.connect(boost::bind(&NetworkPluginServer::handleUserReadyToConnect, this, user));
	user->onPresenceChanged.connect(boost::bind(&NetworkPluginServer::handleUserPresenceChanged, this, user, _1));
	user->onRoomJoined.connect(boost::bind(&NetworkPluginServer::handleRoomJoined, this, user, _1, _2, _3));
	user->onRoomLeft.connect(boost::bind(&NetworkPluginServer::handleRoomLeft, this, user, _1));
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

	Client *c = (Client *) user->getData();
	send(c->connection, message);
}

void NetworkPluginServer::handleUserPresenceChanged(User *user, Swift::Presence::ref presence) {
	if (presence->getShow() == Swift::StatusShow::None)
		return;

	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Status status;
	status.set_username(user->getJID().toBare());
	status.set_status((int) presence->getShow());
	status.set_statusmessage(presence->getStatus());

	std::string message;
	status.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_STATUS_CHANGED);

	Client *c = (Client *) user->getData();
	send(c->connection, message);
}

void NetworkPluginServer::handleRoomJoined(User *user, const std::string &r, const std::string &nickname, const std::string &password) {
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Room room;
	room.set_username(user->getJID().toBare());
	room.set_nickname(nickname);
	room.set_room(r);
	room.set_password(password);

	std::string message;
	room.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_JOIN_ROOM);
 
	Client *c = (Client *) user->getData();
	send(c->connection, message);

	NetworkConversation *conv = new NetworkConversation(user->getConversationManager(), r, true);
	conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
	conv->setNickname(nickname);
}

void NetworkPluginServer::handleRoomLeft(User *user, const std::string &r) {
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Room room;
	room.set_username(user->getJID().toBare());
	room.set_nickname("");
	room.set_room(r);
	room.set_password("");

	std::string message;
	room.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_LEAVE_ROOM);
 
	Client *c = (Client *) user->getData();
	send(c->connection, message);

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(r);
	if (!conv) {
		return;
	}

	user->getConversationManager()->removeConversation(conv);

	delete conv;
}

void NetworkPluginServer::handleUserDestroyed(User *user) {
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Logout logout;
	logout.set_user(user->getJID().toBare());
	logout.set_legacyname(userInfo.uin);

	std::string message;
	logout.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_LOGOUT);
 
	Client *c = (Client *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
	c->users.remove(user);
	if (c->users.size() == 0) {
		std::cout << "DISCONNECTING\n";
		c->connection->disconnect();
		c->connection.reset();
// 		m_clients.erase(user->connection);
	}
}

void NetworkPluginServer::handleMessageReceived(NetworkConversation *conv, boost::shared_ptr<Swift::Message> &msg) {

	boost::shared_ptr<Swift::ChatState> statePayload = msg->getPayload<Swift::ChatState>();
	if (statePayload) {
		pbnetwork::WrapperMessage_Type type = pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED;
		switch (statePayload->getChatState()) {
			case Swift::ChatState::Active:
				type = pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING;
				break;
			case Swift::ChatState::Composing:
				type = pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING;
				break;
			case Swift::ChatState::Paused:
				type = pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED;
				break;
			default:
				break;
		}
		if (type != pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED) {
			pbnetwork::Buddy buddy;
			buddy.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
			buddy.set_buddyname(conv->getLegacyName());

			std::string message;
			buddy.SerializeToString(&message);

			WRAP(message, type);

			Client *c = (Client *) conv->getConversationManager()->getUser()->getData();
			send(c->connection, message);
		}
	}

	if (!msg->getBody().empty()) {
		pbnetwork::ConversationMessage m;
		m.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
		m.set_buddyname(conv->getLegacyName());
		m.set_message(msg->getBody());

		std::string message;
		m.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE);

		Client *c = (Client *) conv->getConversationManager()->getUser()->getData();
		send(c->connection, message);
	}
}

void NetworkPluginServer::handleBuddyRemoved(Buddy *b) {
	User *user = b->getRosterManager()->getUser();

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getJID().toBare());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	buddy.set_groups(b->getGroups().size() == 0 ? "" : b->getGroups()[0]);
	buddy.set_status(Swift::StatusShow::None);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_REMOVED);

	Client *c = (Client *) user->getData();
	send(c->connection, message);
}

void NetworkPluginServer::handleBuddyUpdated(Buddy *b, const Swift::RosterItemPayload &item) {
	User *user = b->getRosterManager()->getUser();

	dynamic_cast<LocalBuddy *>(b)->setAlias(item.getName());
	user->getRosterManager()->storeBuddy(b);

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getJID().toBare());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	buddy.set_groups(b->getGroups().size() == 0 ? "" : b->getGroups()[0]);
	buddy.set_status(Swift::StatusShow::None);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED);

	Client *c = (Client *) user->getData();
	send(c->connection, message);
}

void NetworkPluginServer::handleBuddyAdded(Buddy *buddy, const Swift::RosterItemPayload &item) {
	handleBuddyUpdated(buddy, item);
}

void NetworkPluginServer::handleVCardRequired(User *user, const std::string &name, unsigned int id) {
	std::cout << "VCARD REQUIRED " << name << " " << id << "\n";
	pbnetwork::VCard vcard;
	vcard.set_username(user->getJID().toBare());
	vcard.set_buddyname(name);
	vcard.set_id(id);

	std::string message;
	vcard.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_VCARD);

	Client *c = (Client *) user->getData();
	send(c->connection, message);
}

void NetworkPluginServer::sendPing(Client *c) {

	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_PING);
	wrap.SerializeToString(&message);

	send(c->connection, message);
	c->pongReceived = false;
	std::cout << "SENDING PING\n";
}

NetworkPluginServer::Client *NetworkPluginServer::getFreeClient() {
	for (std::list<Client *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->users.size() < CONFIG_INT(m_config, "service.users_per_backend")) {
			if ((*it)->users.size() + 1 == CONFIG_INT(m_config, "service.users_per_backend")) {
				exec_(CONFIG_STRING(m_config, "service.backend").c_str(), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
			}
			return (*it);
		}
	}
	return NULL;
}

}
