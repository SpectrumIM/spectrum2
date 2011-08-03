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
#include "blockresponder.h"
#include "Swiften/Swiften.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Network/BoostConnectionServer.h"
#include "Swiften/Elements/AttentionPayload.h"
#include "Swiften/Elements/XHTMLIMPayload.h"
#include "pbnetwork.pb.h"
#include "sys/wait.h"
#include "sys/signal.h"
#include "log4cxx/logger.h"
#include "popt.h"

using namespace log4cxx;

namespace Transport {

static unsigned long backend_id;

static LoggerPtr logger = Logger::getLogger("NetworkPluginServer");

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

static pid_t exec_(std::string path, const char *host, const char *port, const char *config) {
	boost::replace_all(path, "BACKEND_ID", boost::lexical_cast<std::string>(backend_id++));
	path += std::string(" --host ") + host + " --port " + port + " " + config;
	LOG4CXX_INFO(logger, "Starting new backend " << path);
	char *p = (char *) malloc(path.size() + 1);
	strcpy(p, path.c_str());
	int argc;
	char **argv;
	poptParseArgvString(p, &argc, (const char ***) &argv);

// 	char *argv[] = {(char*)script_name, '\0'}; 
	pid_t pid = fork();
	if ( pid == 0 ) {
		// child process
		exit(execv(argv[0], argv));
	} else if ( pid < 0 ) {
		// fork failed
	}
	free(p);

	return pid;
}

static void SigCatcher(int n) {
	pid_t result;
	int status;
	while ((result = waitpid(0, &status, WNOHANG)) > 0) {
		if (result != 0) {
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0) {
// 					LOG4CXX_ERROR(logger, "Backend can not be started, exit_code=" << WEXITSTATUS(status));
				}
			}
			else {
// 				LOG4CXX_ERROR(logger, "Backend can not be started");
			}
		}
	}
}

static void handleBuddyPayload(LocalBuddy *buddy, const pbnetwork::Buddy &payload) {
	buddy->setName(payload.buddyname());
	if (!payload.alias().empty()) {
		buddy->setAlias(payload.alias());
	}
	if (!payload.groups().empty()) {
		std::vector<std::string> groups;
		groups.push_back(payload.groups());
		buddy->setGroups(groups);
	}

	buddy->setStatus(Swift::StatusShow((Swift::StatusShow::Type) payload.status()), payload.statusmessage());
	buddy->setIconHash(payload.iconhash());
	buddy->setBlocked(payload.blocked());
}

NetworkPluginServer::NetworkPluginServer(Component *component, Config *config, UserManager *userManager) {
	m_userManager = userManager;
	m_config = config;
	m_component = component;
	m_isNextLongRun = false;
	m_component->m_factory = new NetworkFactory(this);
	m_userManager->onUserCreated.connect(boost::bind(&NetworkPluginServer::handleUserCreated, this, _1));
	m_userManager->onUserDestroyed.connect(boost::bind(&NetworkPluginServer::handleUserDestroyed, this, _1));

	m_pingTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_pingTimer->onTick.connect(boost::bind(&NetworkPluginServer::pingTimeout, this));
	m_pingTimer->start();

	m_collectTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(2*3600000);
	m_collectTimer->onTick.connect(boost::bind(&NetworkPluginServer::collectBackend, this));
	m_collectTimer->start();

	m_vcardResponder = new VCardResponder(component->getIQRouter(), userManager);
	m_vcardResponder->onVCardRequired.connect(boost::bind(&NetworkPluginServer::handleVCardRequired, this, _1, _2, _3));
	m_vcardResponder->onVCardUpdated.connect(boost::bind(&NetworkPluginServer::handleVCardUpdated, this, _1, _2));
	m_vcardResponder->start();

	m_rosterResponder = new RosterResponder(component->getIQRouter(), userManager);
	m_rosterResponder->onBuddyAdded.connect(boost::bind(&NetworkPluginServer::handleBuddyAdded, this, _1, _2));
	m_rosterResponder->onBuddyRemoved.connect(boost::bind(&NetworkPluginServer::handleBuddyRemoved, this, _1));
	m_rosterResponder->onBuddyUpdated.connect(boost::bind(&NetworkPluginServer::handleBuddyUpdated, this, _1, _2));
	m_rosterResponder->start();

	m_blockResponder = new BlockResponder(component->getIQRouter(), userManager);
	m_blockResponder->onBlockToggled.connect(boost::bind(&NetworkPluginServer::handleBlockToggled, this, _1));
	m_blockResponder->start();

	m_server = Swift::BoostConnectionServer::create(Swift::HostAddress(CONFIG_STRING(m_config, "service.backend_host")), boost::lexical_cast<int>(CONFIG_STRING(m_config, "service.backend_port")), component->getNetworkFactories()->getIOServiceThread()->getIOService(), component->m_loop);
	m_server->onNewConnection.connect(boost::bind(&NetworkPluginServer::handleNewClientConnection, this, _1));
	m_server->start();

	LOG4CXX_INFO(logger, "Listening on host " << CONFIG_STRING(m_config, "service.backend_host") << " port " << CONFIG_STRING(m_config, "service.backend_port"));

	signal(SIGCHLD, SigCatcher);

	exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
}

NetworkPluginServer::~NetworkPluginServer() {
	m_pingTimer->stop();
	m_server->stop();
	m_server.reset();
	delete m_component->m_factory;
	delete m_vcardResponder;
	delete m_rosterResponder;
	delete m_blockResponder;
}

void NetworkPluginServer::handleNewClientConnection(boost::shared_ptr<Swift::Connection> c) {
	Backend *client = new Backend;
	client->pongReceived = -1;
	client->connection = c;
	client->res = 0;
	client->init_res = 0;
	client->shared = 0;
	client->acceptUsers = !m_isNextLongRun;
	client->longRun = m_isNextLongRun;

	LOG4CXX_INFO(logger, "New" + (client->longRun ? std::string(" long-running") : "") +  " backend " << client << " connected. Current backend count=" << (m_clients.size() + 1));

	if (m_clients.size() == 0) {
		// first backend connected, start the server, we're ready.
		m_component->start();
	}

	m_clients.push_front(client);

	c->onDisconnected.connect(boost::bind(&NetworkPluginServer::handleSessionFinished, this, client));
	c->onDataRead.connect(boost::bind(&NetworkPluginServer::handleDataRead, this, client, _1));
	sendPing(client);
	client->pongReceived = -1;

	// some users are in queue waiting for this backend
	while(!m_waitingUsers.empty()) {
		// There's no new backend, so stop associating users and wait for new backend,
		// which has been already spawned in getFreeClient() call.
		if (getFreeClient() == NULL)
			break;

		User *u = m_waitingUsers.front();
		m_waitingUsers.pop_front();

		LOG4CXX_INFO(logger, "Associating " << u->getJID().toString() << " with this backend");

		// associate backend with user
		handleUserCreated(u);

		// connect user if it's ready
		if (u->isReadyToConnect()) {
			handleUserReadyToConnect(u);
		}

	}
}

void NetworkPluginServer::handleSessionFinished(Backend *c) {
	LOG4CXX_INFO(logger, "Backend " << c << " disconnected. Current backend count=" << (m_clients.size() - 1));
	for (std::list<User *>::const_iterator it = c->users.begin(); it != c->users.end(); it++) {
		LOG4CXX_ERROR(logger, "Backend " << c << " disconnected (probably crashed) with active user " << (*it)->getJID().toString());
		(*it)->setData(NULL);
		(*it)->handleDisconnected("Internal Server Error, please reconnect.");
	}

// 	c->connection->onDisconnected.disconnect_all_slots();
// 	c->connection->onDataRead.disconnect_all_slots();

	m_clients.remove(c);
	delete c;

	// Execute new session only if there's no free one after this crash/disconnection
// 	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
// 		if ((*it)->users.size() < CONFIG_INT(m_config, "service.users_per_backend")) {
// 			return;
// 		}
// 	}
// 	exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
}

void NetworkPluginServer::handleConnectedPayload(const std::string &data) {
	pbnetwork::Connected payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.user());
	if (!user) {
		return;
	}

	user->setConnected(true);
	m_component->m_userRegistry->onPasswordValid(payload.user());
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

	boost::shared_ptr<Swift::VCard> vcard(new Swift::VCard());
	vcard->setFullName(payload.fullname());
	vcard->setPhoto(Swift::createByteArray(payload.photo()));
	vcard->setNickname(payload.nickname());

	m_vcardResponder->sendVCard(payload.id(), vcard);
}

void NetworkPluginServer::handleAuthorizationPayload(const std::string &data) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(user->getJID());
	std::string name = payload.buddyname();

// 	name = Swift::JID::getEscapedNode(name)

	if (name.find_last_of("@") != std::string::npos) {
		name.replace(name.find_last_of("@"), 1, "%");
	}

	response->setFrom(Swift::JID(name, m_component->getJID().toString()));
	response->setType(Swift::Presence::Subscribe);
	m_component->getStanzaChannel()->sendPresence(response);
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

	user->updateLastActivity();

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	if (subject) {
		msg->setSubject(payload.message());
	}
	else {
		msg->setBody(payload.message());
	}

	if (!payload.xhtml().empty()) {
		msg->addPayload(boost::make_shared<Swift::XHTMLIMPayload>(payload.xhtml()));
	}

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
		conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
	}

	conv->handleMessage(msg, payload.nickname());
}

void NetworkPluginServer::handleAttentionPayload(const std::string &data) {
	pbnetwork::ConversationMessage payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(payload.message());
	msg->addPayload(boost::make_shared<Swift::AttentionPayload>());

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
		conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
	}

	conv->handleMessage(msg);
}

void NetworkPluginServer::handleStatsPayload(Backend *c, const std::string &data) {
	pbnetwork::Stats payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}
	c->res = payload.res();
	c->init_res = payload.init_res();
	c->shared = payload.shared();
}

void NetworkPluginServer::handleDataRead(Backend *c, const Swift::SafeByteArray &data) {
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
			case pbnetwork::WrapperMessage_Type_TYPE_AUTH_REQUEST:
				handleAuthorizationPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_ATTENTION:
				handleAttentionPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_STATS:
				handleStatsPayload(c, wrapper.payload());
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
	// TODO: move to separate timer, those 2 loops could be expensive
	time_t now = time(NULL);
	std::vector<User *> usersToMove;
	unsigned long diff = CONFIG_INT(m_config, "service.idle_reconnect_time");
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->longRun) {
			continue;
		}

		BOOST_FOREACH(User *u, (*it)->users) {
			if (now - u->getLastActivity() > diff) {
				usersToMove.push_back(u);
			}
		}
	}

	BOOST_FOREACH(User *u, usersToMove) {
		LOG4CXX_INFO(logger, "Moving user " << u->getJID().toString() << " to long-running backend");
		if (!moveToLongRunBackend(u))
			break;
	}
	

	// check ping responses
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->pongReceived || (*it)->pongReceived == -1) {
			sendPing((*it));
		}
		else {
			LOG4CXX_INFO(logger, "Disconnecting backend " << (*it) << ". PING response not received.");
			(*it)->connection->disconnect();
			(*it)->connection.reset();
// 			handleSessionFinished((*it));
		}

		if ((*it)->users.size() == 0) {
			LOG4CXX_INFO(logger, "Disconnecting backend " << (*it) << ". There are no users.");
			(*it)->connection->disconnect();
			(*it)->connection.reset();
		}
	}
	m_pingTimer->start();
}

void NetworkPluginServer::collectBackend() {
	LOG4CXX_INFO(logger, "Collect backend called, finding backend which will be set to die");
	unsigned long max = 0;
	Backend *backend = NULL;
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->res > max) {
			max = (*it)->res;
			backend = (*it);
		}
	}

	if (backend) {
		m_collectTimer->start();
		LOG4CXX_INFO(logger, "Backend " << backend << "is set to die");
		backend->acceptUsers = false;
	}
}

bool NetworkPluginServer::moveToLongRunBackend(User *user) {
	// Check if user has already some backend
	Backend *old = (Backend *) user->getData();
	if (!old) {
		LOG4CXX_INFO(logger, "User " << user->getJID().toString() << " does not have old backend. Not moving.");
		return true;
	}

	// if he's already on long run, do nothing
	if (old->longRun) {
		LOG4CXX_INFO(logger, "User " << user->getJID().toString() << " is already on long-running backend. Not moving.");
		return true;
	}

	// Get free longrun backend, if there's no longrun backend, create one and wait
	// for its connection
	Backend *backend = getFreeClient(false, true);
	if (!backend) {
		LOG4CXX_INFO(logger, "No free long-running backend for user " << user->getJID().toString() << ". Will try later");
		return false;
	}

	// old backend will trigger disconnection which has to be ignored to keep user online
	user->setIgnoreDisconnect(true);

	// remove user from the old backend
	// If backend is empty, it will be collected by pingTimeout
	old->users.remove(user);

	// switch to new backend and connect
	user->setData(backend);
	backend->users.push_back(user);

	// connect him
	handleUserReadyToConnect(user);
	return true;
}

void NetworkPluginServer::handleUserCreated(User *user) {
	Backend *c = getFreeClient();

	if (!c) {
		LOG4CXX_INFO(logger, "There is no backend to handle user " << user->getJID().toString() << ". Adding him to queue.");
		m_waitingUsers.push_back(user);
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

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
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

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
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
 
	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
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
 
	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(r);
	if (!conv) {
		return;
	}

	user->getConversationManager()->removeConversation(conv);

	delete conv;
}

void NetworkPluginServer::handleUserDestroyed(User *user) {
	std::cout << "HANDLE_DESTROYED\n";
	m_waitingUsers.remove(user);
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Logout logout;
	logout.set_user(user->getJID().toBare());
	logout.set_legacyname(userInfo.uin);

	std::string message;
	logout.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_LOGOUT);
 
	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
	c->users.remove(user);
	if (c->users.size() == 0) {
		LOG4CXX_INFO(logger, "Disconnecting backend " << c << ". There are no users.");
		c->connection->disconnect();
		c->connection.reset();
		
// 		handleSessionFinished(c);
// 		m_clients.erase(user->connection);
	}
}

void NetworkPluginServer::handleMessageReceived(NetworkConversation *conv, boost::shared_ptr<Swift::Message> &msg) {
	conv->getConversationManager()->getUser()->updateLastActivity();
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

			Backend *c = (Backend *) conv->getConversationManager()->getUser()->getData();
			if (!c) {
				return;
			}
			send(c->connection, message);
		}
	}

	boost::shared_ptr<Swift::AttentionPayload> attentionPayload = msg->getPayload<Swift::AttentionPayload>();
	if (attentionPayload) {
		pbnetwork::ConversationMessage m;
		m.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
		m.set_buddyname(conv->getLegacyName());
		m.set_message(msg->getBody());

		std::string message;
		m.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_ATTENTION);

		Backend *c = (Backend *) conv->getConversationManager()->getUser()->getData();
		send(c->connection, message);
		return;
	}
	

	std::string xhtml;
	boost::shared_ptr<Swift::XHTMLIMPayload> xhtmlPayload = msg->getPayload<Swift::XHTMLIMPayload>();
	if (xhtmlPayload) {
		xhtml = xhtmlPayload->getBody();
	}

	// Send normal message
	if (!msg->getBody().empty() || !xhtml.empty()) {
		pbnetwork::ConversationMessage m;
		m.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
		m.set_buddyname(conv->getLegacyName());
		m.set_message(msg->getBody());
		m.set_xhtml(xhtml);

		std::string message;
		m.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE);

		Backend *c = (Backend *) conv->getConversationManager()->getUser()->getData();
		if (!c) {
			return;
		}
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

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::handleBuddyUpdated(Buddy *b, const Swift::RosterItemPayload &item) {
	User *user = b->getRosterManager()->getUser();

	dynamic_cast<LocalBuddy *>(b)->setAlias(item.getName());
	dynamic_cast<LocalBuddy *>(b)->setGroups(item.getGroups());
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

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::handleBuddyAdded(Buddy *buddy, const Swift::RosterItemPayload &item) {
	handleBuddyUpdated(buddy, item);
}

void NetworkPluginServer::handleBlockToggled(Buddy *b) {
	User *user = b->getRosterManager()->getUser();

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getJID().toBare());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	buddy.set_groups(b->getGroups().size() == 0 ? "" : b->getGroups()[0]);
	buddy.set_status(Swift::StatusShow::None);
	buddy.set_blocked(not b->isBlocked());

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}


void NetworkPluginServer::handleVCardUpdated(User *user, boost::shared_ptr<Swift::VCard> v) {
	pbnetwork::VCard vcard;
	vcard.set_username(user->getJID().toBare());
	vcard.set_buddyname("");
	vcard.set_id(0);
	vcard.set_photo(&v->getPhoto()[0], v->getPhoto().size());

	std::string message;
	vcard.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_VCARD);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::handleVCardRequired(User *user, const std::string &name, unsigned int id) {
	pbnetwork::VCard vcard;
	vcard.set_username(user->getJID().toBare());
	vcard.set_buddyname(name);
	vcard.set_id(id);

	std::string message;
	vcard.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_VCARD);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::sendPing(Backend *c) {

	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_PING);
	wrap.SerializeToString(&message);

	if (c->connection) {
		send(c->connection, message);
		c->pongReceived = false;
	}
// 	LOG4CXX_INFO(logger, "PING to " << c);
}

NetworkPluginServer::Backend *NetworkPluginServer::getFreeClient(bool acceptUsers, bool longRun) {
	NetworkPluginServer::Backend *c = NULL;
// 	bool spawnNew = false;
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		// This backend is free.
		if ((*it)->acceptUsers == acceptUsers && (*it)->users.size() < CONFIG_INT(m_config, "service.users_per_backend") && (*it)->connection && (*it)->longRun == longRun) {
			c = *it;
			if (!CONFIG_BOOL(m_config, "service.reuse_old_backends")) {
				if (c->users.size() + 1 >= CONFIG_INT(m_config, "service.users_per_backend")) {
					c->acceptUsers = false;
				}
			}
			break;
		}
	}

	if (c == NULL) {
		m_isNextLongRun = longRun;
		exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getConfigFile().c_str());
	}

	return c;
}

}
