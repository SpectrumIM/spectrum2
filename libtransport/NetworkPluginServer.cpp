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

#include "transport/NetworkPluginServer.h"
#include "transport/Event.h"
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/UserManager.h"
#include "transport/ConversationManager.h"
#include "transport/LocalBuddy.h"
#include "transport/Config.h"
#include "transport/Conversation.h"
#include "transport/Logging.h"
#include "transport/AdminInterface.h"
#include "transport/Frontend.h"
#include "transport/Factory.h"
#include "transport/protocol.pb.h"
#include "transport/Util.h"
#include "Swiften/Network/ConnectionServerFactory.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include "transport/utf8.h"

#ifdef _WIN32
#include "windows.h"
#include <stdint.h>
#else
#include <arpa/inet.h>
#include "sys/wait.h"
#include "sys/signal.h"
#include <sys/types.h>
#include <signal.h>
#include "popt.h"
#include <dirent.h>
#endif

using namespace Transport::Util;

namespace Transport {

static unsigned long backend_id;
static unsigned long bytestream_id;

DEFINE_LOGGER(networkPluginServerLogger, "NetworkPluginServer");

static NetworkPluginServer *_server;

class NetworkFactory : public Factory {
	public:
		NetworkFactory(NetworkPluginServer *nps, Component *component) {
			m_nps = nps;
			m_component = component;
		}

		virtual ~NetworkFactory() {}

		// Creates new conversation; the frontend shows it, we send its messages to the backend.
		Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMuc) {
			Conversation *conv = m_component->getFrontend()->createConversation(conversationManager, legacyName, isMuc);
			conv->onMessageToSend = [nps = m_nps](Conversation *c, pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &msg) { nps->handleMessageReceived(c, type, msg); };
			return conv;
		}

		// Creates new LocalBuddy
		Buddy *createBuddy(RosterManager *rosterManager, const BuddyInfo &buddyInfo) {
			LocalBuddy *buddy = new LocalBuddy(rosterManager, buddyInfo.id, buddyInfo.legacyName, buddyInfo.alias, buddyInfo.groups, (BuddyFlag) buddyInfo.flags);
			if (!buddy->isValid()) {
				delete buddy;
				return NULL;
			}
			if (buddyInfo.subscription == "both") {
				buddy->setSubscription(Buddy::Both);
			}
			else {
				buddy->setSubscription(Buddy::Ask);
			}
			if (buddyInfo.settings.find("icon_hash") != buddyInfo.settings.end())
				buddy->setIconHash(buddyInfo.settings.find("icon_hash")->second.s);
			return buddy;
		}

	private:
		NetworkPluginServer *m_nps;
		Component *m_component;
};

// Wraps google protobuf payload into WrapperMessage and serialize it to string
#define WRAP(MESSAGE, TYPE) 	pbnetwork::WrapperMessage wrap; \
	wrap.set_type(TYPE); \
	wrap.set_payload(MESSAGE); \
	wrap.SerializeToString(&MESSAGE);

// Executes new backend
static unsigned long exec_(const std::string& exePath, const char *host, const char *port, const char *log_id, const char *cmdlineArgs) {
	// BACKEND_ID is replaced with unique ID. The ID is increasing for every backend.
	std::string finalExePath = Util::replaceAllCopy(exePath, "BACKEND_ID", std::to_string(backend_id++));

#ifdef _WIN32
	// Add host and port.
	std::ostringstream fullCmdLine;
	fullCmdLine << "\"" << finalExePath << "\" --host " << host << " --port " << port;

	if (cmdlineArgs)
		fullCmdLine << " " << cmdlineArgs;

	LOG4CXX_INFO(networkPluginServerLogger, "Starting new backend " << fullCmdLine.str());

	// We must provide a non-const buffer to CreateProcess below
	std::vector<wchar_t> rawCommandLineArgs( fullCmdLine.str().size() + 1 );
	wcscpy_s(&rawCommandLineArgs[0], rawCommandLineArgs.size(), utf8ToUtf16(fullCmdLine.str()).c_str());

	STARTUPINFO         si;
	PROCESS_INFORMATION pi;

	ZeroMemory (&si, sizeof(si));
	si.cb=sizeof (si);

	if (! CreateProcess(
		utf8ToUtf16(finalExePath).c_str(),
		&rawCommandLineArgs[0],
		0,                    // process attributes
		0,                    // thread attributes
		0,                    // inherit handles
		0,                    // creation flags
		0,                    // environment
		0,                    // cwd
		&si,
		&pi
		)
	)  {
		LOG4CXX_ERROR(networkPluginServerLogger, "Could not start process");
	}

	return 0;
#else
	// Add host and port.
	finalExePath += std::string(" --host ") + host + " --port " + port + " --service.backend_id=" + log_id + " " + cmdlineArgs;
	LOG4CXX_INFO(networkPluginServerLogger, "Starting new backend " << finalExePath);

	// Create array of char * from string using -lpopt library
	char *p = (char *) malloc(finalExePath.size() + 1);
	strcpy(p, finalExePath.c_str());
	int argc;
	char **argv;
	poptParseArgvString(p, &argc, (const char ***) &argv);

	// fork and exec
	pid_t pid = fork();
	if ( pid == 0 ) {
		setsid();
		// Close all open file descriptors beyond stdin/stdout/stderr.
		// Prefer /proc/self/fd (Linux) — sysconf(_SC_OPEN_MAX) can return
		// ~1 billion in Docker containers, which would hang for minutes.
		DIR *d = opendir("/proc/self/fd");
		if (d) {
			int dir_fd = dirfd(d);
			struct dirent *ent;
			while ((ent = readdir(d)) != NULL) {
				if (ent->d_name[0] == '.') continue;
				int fd = atoi(ent->d_name);
				if (fd > 2 && fd != dir_fd) {
					close(fd);
				}
			}
			closedir(d);
		} else {
			// Fallback for non-Linux (macOS, BSD): sysconf is safe there
			int maxfd = sysconf(_SC_OPEN_MAX);
			for (int fd = 3; fd < maxfd; fd++) {
				close(fd);
			}
		}
		// child process
		errno = 0;
		int ret = execv(argv[0], argv);
		if (ret == -1) {
			exit(errno);
		}
		exit(0);
	} else if ( pid < 0 ) {
		LOG4CXX_ERROR(networkPluginServerLogger, "Fork failed");
	}
	free(p);

	return (unsigned long) pid;
#endif
}

#ifndef _WIN32
static void SigCatcher(int n) {
	pid_t result;
	int status;
	// Read exit code from all children to not have zombies arround
	// WARNING: Do not put LOG4CXX_ here, because it can lead to deadlock
	while ((result = waitpid(-1, &status, WNOHANG)) > 0) {
		if (result != 0) {
			_server->handlePIDTerminated((unsigned long)result);
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0) {
// 					LOG4CXX_ERROR(networkPluginServerLogger, "Backend can not be started, exit_code=" << WEXITSTATUS(status));
				}
			}
			else {
// 				LOG4CXX_ERROR(networkPluginServerLogger, "Backend can not be started");
			}
		}
	}
}
#endif

static void handleBuddyPayload(LocalBuddy *buddy, const pbnetwork::Buddy &payload) {
	// Set alias only if it's not empty. Backends are allowed to send empty alias if it has
	// not changed.
	if (!payload.alias().empty()) {
		buddy->setAlias(payload.alias());
	}

	// Change groups if it's not empty. The same as above...
	std::vector<std::string> groups;
	for (int i = 0; i < payload.group_size(); i++) {
		std::string group;
		utf8::replace_invalid(payload.group(i).begin(), payload.group(i).end(), std::back_inserter(group), '_');
		groups.push_back(group);
	}
	if (!groups.empty()) {
		buddy->setGroups(groups);
	}

	buddy->setStatus(payload.status(), payload.statusmessage());
	buddy->setIconHash(payload.iconhash());
	buddy->setBlocked(payload.blocked());
}

NetworkPluginServer::NetworkPluginServer(Component *component, Config *config, UserManager *userManager) {
	_server = this;
	m_userManager = userManager;
	m_config = config;
	m_component = component;
	m_isNextLongRun = false;
	m_adminInterface = NULL;
	m_startingBackend = false;
	m_lastLogin = 0;
	m_firstPong = true;
	m_component->m_factory = new NetworkFactory(this, component);
	m_userManager->onUserCreated = [this](User *user) { handleUserCreated(user); };
	m_userManager->onUserDestroyed = [this](User *user) { handleUserDestroyed(user); };

	m_pingTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_pingTimer->onTick.connect(boost::bind(&NetworkPluginServer::pingTimeout, this));
	m_pingTimer->start();

	m_loginTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(CONFIG_INT(config, "service.login_delay") * 1000);
	m_loginTimer->onTick.connect(boost::bind(&NetworkPluginServer::loginDelayFinished, this));
	m_loginTimer->start();

	if (CONFIG_INT(m_config, "service.memory_collector_time") != 0) {
		m_collectTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(CONFIG_INT(m_config, "service.memory_collector_time"));
		m_collectTimer->onTick.connect(boost::bind(&NetworkPluginServer::collectBackend, this));
		m_collectTimer->start();
	}

	Frontend *frontend = m_component->getFrontend();
	frontend->onVCardRequired = [this](User *user, const std::string &name, unsigned int id) { handleVCardRequired(user, name, id); };
	frontend->onVCardUpdated = [this](User *user, const pbnetwork::VCard &vcard) { handleVCardUpdated(user, vcard); };

	frontend->onBuddyAdded = [this](Buddy *buddy, const pbnetwork::Buddy &item) { handleBuddyAdded(buddy, item); };
	frontend->onBuddyRemoved = [this](Buddy *buddy) { handleBuddyRemoved(buddy); };
	frontend->onBuddyUpdated = [this](Buddy *buddy, const pbnetwork::Buddy &item) { handleBuddyUpdated(buddy, item); };

	frontend->onRawXMLReceived = [this](User *user, const std::string &xml) { handleRawXMLReceived(user, xml); };

	m_server = component->getNetworkFactories()->getConnectionServerFactory()->createConnectionServer(*(Swift::HostAddress::fromString(CONFIG_STRING_DEFAULTED(m_config, "service.backend_host", "127.0.0.1"))), std::stoi(CONFIG_STRING(m_config, "service.backend_port")));
	m_server->onNewConnection.connect(boost::bind(&NetworkPluginServer::handleNewClientConnection, this, _1));
}

NetworkPluginServer::~NetworkPluginServer() {
#ifndef _WIN32
	signal(SIGCHLD, SIG_IGN);
#endif

	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		LOG4CXX_INFO(networkPluginServerLogger, "Stopping backend " << *it);
		std::string message;
		pbnetwork::WrapperMessage wrap;
		wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_EXIT);
		wrap.SerializeToString(&message);

		Backend *c = (Backend *) *it;
		send(c->connection, message);
	}

	m_pingTimer->stop();
	m_server->stop();
	m_server.reset();
	delete m_component->m_factory;
}

void NetworkPluginServer::start() {
	m_server->start();

	LOG4CXX_INFO(networkPluginServerLogger, "Listening on host " << CONFIG_STRING(m_config, "service.backend_host") << " port " << CONFIG_STRING(m_config, "service.backend_port"));

	while (true) {
		unsigned long pid = exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), "1", m_config->getCommandLineArgs().c_str());
		LOG4CXX_INFO(networkPluginServerLogger, "Tried to spawn first backend with pid " << pid);
		LOG4CXX_INFO(networkPluginServerLogger, "Backend should now connect to Spectrum2 instance. Spectrum2 won't accept any connection before backend connects");

#ifndef _WIN32
		// wait if the backend process will still be alive after 1 second
		sleep(1);
		pid_t result;
		int status;
		result = waitpid(-1, &status, WNOHANG);
		if (result != 0) {
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0) {
					if (status == 254) {
						LOG4CXX_ERROR(networkPluginServerLogger, "Backend can not be started, because it needs database to store data, but the database backend is not configured.");
					}
					else {
						LOG4CXX_ERROR(networkPluginServerLogger, "Backend can not be started, exit_code=" << WEXITSTATUS(status) << ", possible error: " << strerror(WEXITSTATUS(status)));
						if (WEXITSTATUS(status) == ENOENT) {
							LOG4CXX_ERROR(networkPluginServerLogger, "This usually means the path to backend executable defined in config file as '[service] backend=\"...\"' is wrong or the executable does not exists.");
						}

					}
					LOG4CXX_ERROR(networkPluginServerLogger, "Check backend log for more details");
					continue;
				}
			}
			else {
				LOG4CXX_ERROR(networkPluginServerLogger, "Backend can not be started");
				continue;
			}
		}

		m_pids.push_back(pid);

		signal(SIGCHLD, SigCatcher);
#endif
		// quit the while loop
		break;
	}
}

void NetworkPluginServer::loginDelayFinished() {
	m_loginTimer->stop();
	connectWaitingUsers();
}

void NetworkPluginServer::handleNewClientConnection(std::shared_ptr<Swift::Connection> c) {
	// Create new Backend instance
	Backend *client = new Backend;
	client->pongReceived = -1;
	client->connection = c;
	client->res = 0;
	client->init_res = 0;
	client->shared = 0;
	// Until we receive first PONG from backend, backend is in willDie state.
	client->willDie = true;
	// Backend does not accept new clients automatically if it's long-running
	client->acceptUsers = !m_isNextLongRun;
	client->longRun = m_isNextLongRun;

	m_startingBackend = false;

	LOG4CXX_INFO(networkPluginServerLogger, "New" + (client->longRun ? std::string(" long-running") : "") +  " backend " << client << " connected. Current backend count=" << (m_clients.size() + 1));

	m_clients.push_front(client);

	c->onDisconnected.connect(boost::bind(&NetworkPluginServer::handleSessionFinished, this, client));
	c->onDataRead.connect(boost::bind(&NetworkPluginServer::handleDataRead, this, client, _1));
	sendPing(client);

	// sendPing sets pongReceived to 0, but we want to have it -1 to ignore this backend
	// in first ::pingTimeout call, because it can be called right after this function
	// and backend wouldn't have any time to response to ping.
	client->pongReceived = -1;
}

void NetworkPluginServer::handleSessionFinished(Backend *c) {
	LOG4CXX_INFO(networkPluginServerLogger, "Backend " << c << " (ID=" << c->id << ") disconnected. Current backend count=" << (m_clients.size() - 1));

	// This backend will do, so we can't reconnect users to it in User::handleDisconnected call
	c->willDie = true;

	// If there are users associated with this backend, it must have crashed, so print error output
	// and disconnect users
	if (!c->users.empty()) {
		m_crashedBackends.push_back(c->id);
	}

	for (std::list<User *>::const_iterator it = c->users.begin(); it != c->users.end(); it++) {
		LOG4CXX_ERROR(networkPluginServerLogger, "Backend " << c << " (ID=" << c->id << ") disconnected (probably crashed) with active user " << (*it)->getUserName());
		(*it)->setData(NULL);
		(*it)->handleDisconnected("Internal Server Error, please reconnect.");
	}

	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_EXIT);
	wrap.SerializeToString(&message);

	send(c->connection, message);

	c->connection->onDisconnected.disconnect_all_slots();
	c->connection->onDataRead.disconnect_all_slots();
	c->connection->disconnect();
	c->connection.reset();

	m_clients.remove(c);
	delete c;
}

void NetworkPluginServer::handleConnectedPayload(const std::string &data) {
	pbnetwork::Connected payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.user());
	if (!user) {
		LOG4CXX_ERROR(networkPluginServerLogger, "Connected payload received for unknown user " << payload.user());
		return;
	}

	user->setConnected(true);
	m_component->getFrontend()->handleLoginResult(payload.user(), true, "");
}

void NetworkPluginServer::handleDisconnectedPayload(const std::string &data) {
	pbnetwork::Disconnected payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	m_component->getFrontend()->handleLoginResult(payload.user(), false, payload.message());

	User *user = m_userManager->getUser(payload.user());
	if (!user) {
		return;
	}
	user->handleDisconnected(payload.message(), (pbnetwork::ConnectionError) payload.error());
}

void NetworkPluginServer::handleVCardPayload(const std::string &data) {
	pbnetwork::VCard payload;
	if (payload.ParseFromString(data) == false) {
		std::cout << "PARSING ERROR\n";
		// TODO: ERROR
		return;
	}

	pbnetwork::VCard vcard(payload);
	std::string field;

	utf8::replace_invalid(payload.fullname().begin(), payload.fullname().end(), std::back_inserter(field), '_');
	vcard.set_fullname(field);

	field.clear();

	utf8::replace_invalid(payload.nickname().begin(), payload.nickname().end(), std::back_inserter(field), '_');
	vcard.set_nickname(field);

	m_userManager->sendVCard(payload.id(), vcard);
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

	user->getRosterManager()->doRequestAuthorization(payload.buddyname());
}

void NetworkPluginServer::handleChatStatePayload(const std::string &data, pbnetwork::WrapperMessage_Type type) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	// We're not creating new Conversation just because of chatstates.
	// Some networks/clients spams with chatstates a lot and it leads to bigger memory usage.
	Conversation *conv = user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		return;
	}

	// Forward chatstate
	pbnetwork::ConversationMessage msg;
	msg.set_username(payload.username());
	msg.set_buddyname(payload.buddyname());
	msg.set_message("");
	conv->handleMessage(type, msg);
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
	}
	else {
		if (payload.buddyname() == user->getUserInfo().uin) {
			return;
		}

		std::vector<std::string> groups;
		for (int i = 0; i < payload.group_size(); i++) {
			groups.push_back(payload.group(i));
		}
		if (CONFIG_BOOL_DEFAULTED(m_config, "service.jid_escaping", true)) {
			buddy = new LocalBuddy(user->getRosterManager(), -1, payload.buddyname(), payload.alias(), groups, BUDDY_JID_ESCAPING);
		}
		else {
			buddy = new LocalBuddy(user->getRosterManager(), -1, payload.buddyname(), payload.alias(), groups, BUDDY_NO_FLAG);
		}
		if (!buddy->isValid()) {
			delete buddy;
			return;
		}

		buddy->setBlocked(payload.blocked());
		user->getRosterManager()->setBuddy(buddy);
		buddy->setStatus(payload.status(), payload.statusmessage());
		buddy->setIconHash(payload.iconhash());
	}
}

void NetworkPluginServer::handleBuddyRemovedPayload(const std::string &data) {
	pbnetwork::Buddy payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	user->getRosterManager()->removeBuddy(payload.buddyname());
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

	Conversation *conv = user->getConversationManager()->getConversation(payload.room());
	if (!conv) {
		return;
	}

	conv->handleParticipantChanged(payload);
}

void NetworkPluginServer::handleRoomChangedPayload(const std::string &data) {
	pbnetwork::Room payload;
	if (payload.ParseFromString(data) == false) {
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user) {
		LOG4CXX_ERROR(networkPluginServerLogger, "RoomChangePayload for unknown user " << payload.username());
		return;
	}

	Conversation *conv = user->getConversationManager()->getConversation(payload.room());
	if (!conv) {
		LOG4CXX_ERROR(networkPluginServerLogger, "RoomChangePayload for unknown conversation " << payload.room());
		return;
	}

	conv->setNickname(payload.nickname());
}

Conversation *NetworkPluginServer::getOrCreateConversation(User *user, const std::string &name) {
	Conversation *conv = user->getConversationManager()->getConversation(name);
	if (!conv) {
		conv = m_component->getFactory()->createConversation(user->getConversationManager(), name, false);
		user->getConversationManager()->addConversation(conv);
	}
	return conv;
}

void NetworkPluginServer::handleConvMessagePayload(const std::string &data, bool subject) {
	pbnetwork::ConversationMessage payload;
	LOG4CXX_TRACE(networkPluginServerLogger, "handleConvMessagePayload");

	if (payload.ParseFromString(data) == false) {
		LOG4CXX_ERROR(networkPluginServerLogger, "handleConvMessagePayload: cannot parse payload");
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user) {
		LOG4CXX_ERROR(networkPluginServerLogger, "handleConvMessagePayload: unknown username " << payload.username());
		return;
	}

	// Message from legacy network triggers network acticity
	user->updateLastActivity();

	Conversation *conv = user->getConversationManager()->getConversation(payload.buddyname());

	// We can't create Conversation for payload with nickname, because this means the message is from room,
	// but this user is not in any room, so it's OK to just reject this message
	if (!conv && !payload.nickname().empty()) {
		LOG4CXX_WARN(networkPluginServerLogger, "handleConvMessagePayload: No conversation with name " << payload.buddyname());
		return;
	}

	if (conv && payload.pm()) {
		conv = user->getConversationManager()->getConversation(payload.buddyname() + "/" + payload.nickname());
		if (!conv) {
			conv = m_component->getFactory()->createConversation(user->getConversationManager(), payload.nickname(), false);
			std::string name = payload.buddyname();
			conv->setRoom(name);
			conv->setNickname(payload.buddyname() + "/" + payload.nickname());

			user->getConversationManager()->addConversation(conv);
		}
	}

	// Create new Conversation if it does not exist
	if (!conv) {
		conv = getOrCreateConversation(user, payload.buddyname());
	}

	conv->handleMessage(subject ? pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED : pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE, payload);

	m_userManager->messageToXMPPSent();
}

void NetworkPluginServer::handleConvMessageAckPayload(const std::string &data) {
	pbnetwork::ConversationMessage payload;

	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	if (payload.id().empty()) {
		LOG4CXX_WARN(networkPluginServerLogger, "Received message ack with empty ID, not forwarding to XMPP.");
		return;
	}

	Conversation *conv = user->getConversationManager()->getConversation(payload.buddyname());

	// Receipts don't create conversation
	if (!conv) {
		return;
	}

	// Forward it
	conv->handleMessage(pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE_ACK, payload);
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

	// Attentions trigger new Conversation creation
	Conversation *conv = getOrCreateConversation(user, payload.buddyname());

	conv->handleMessage(pbnetwork::WrapperMessage_Type_TYPE_ATTENTION, payload);
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
	c->id = payload.id();
}

void NetworkPluginServer::connectWaitingUsers() {
	// some users are in queue waiting for this backend
	while (!m_waitingUsers.empty()) {
		// There's no new backend, so stop associating users and wait for new backend,
		// which has been already spawned in getFreeClient() call.
		if (getFreeClient(true, false, true) == NULL)
			break;

		User *u = m_waitingUsers.front();
		m_waitingUsers.pop_front();

		LOG4CXX_INFO(networkPluginServerLogger, "Associating " << u->getUserName() << " with this backend");

		// associate backend with user
		handleUserCreated(u);

		// connect user if it's ready
		if (u->isReadyToConnect()) {
			handleUserReadyToConnect(u);
		}
	}
}

void NetworkPluginServer::handlePongReceived(Backend *c) {
	// This could be first PONG from the backend
	if (c->pongReceived == -1) {
		// Backend is fully ready to handle requests
		c->willDie = false;

		if (m_firstPong) {
			// first backend connected, start the server, we're ready.
			m_component->start();
			m_firstPong = false;
		}

		connectWaitingUsers();
	}

	c->pongReceived = true;
}

void NetworkPluginServer::handleQueryPayload(Backend *b, const std::string &data) {
	pbnetwork::BackendConfig payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	if (!m_adminInterface) {
		return;
	}

	pbnetwork::BackendConfig response;
	response.set_config(m_adminInterface->handleQuery(payload.config()));

	std::string message;
	response.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_QUERY);

	send(b->connection, message);
}

void NetworkPluginServer::handleBackendConfigPayload(const std::string &data) {
	pbnetwork::BackendConfig payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	m_config->updateBackendConfig(payload.config());
}

void NetworkPluginServer::handleRoomListPayload(const std::string &data) {
	pbnetwork::RoomList payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	if (!payload.user().empty()) {
		User *user = m_userManager->getUser(payload.user());
		if (!user) {
			LOG4CXX_ERROR(networkPluginServerLogger, "Room list payload received for unknown user " << payload.user());
			return;
		}

		user->clearRoomList();
		for (int i = 0; i < payload.room_size() && i < payload.name_size(); i++) {
			user->addRoomToRoomList(payload.room(i), payload.name(i));
		}
	}
	else {
		m_component->getFrontend()->clearRoomList();
		for (int i = 0; i < payload.room_size() && i < payload.name_size(); i++) {
			m_component->getFrontend()->addRoomToRoomList(payload.room(i), payload.name(i));
		}
	}
}

void NetworkPluginServer::handleRawXML(const std::string &xml) {
	m_component->getFrontend()->sendRawXML(xml);
}

void NetworkPluginServer::handleRawXMLReceived(User *user, const std::string &xml) {
	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_RAW_XML, xml);
}

void NetworkPluginServer::handleDataRead(Backend *c, std::shared_ptr<Swift::SafeByteArray> data) {
	// Append data to buffer
	c->data.insert(c->data.end(), data->begin(), data->end());

	// Parse data while there are some
	while (c->data.size() != 0) {
		// expected_size of wrapper message
		unsigned int expected_size;

		// if data is >= 4, we have whole header and we can
		// read expected_size.
		if (c->data.size() >= 4) {
			expected_size = *((unsigned int*) &c->data[0]);
			expected_size = ntohl(expected_size);
			// If we don't have whole wrapper message, wait for next
			// handleDataRead call.
			if (c->data.size() - 4 < expected_size)
				return;
		}
		else {
			return;
		}

		// Parse wrapper message and erase it from buffer.
		pbnetwork::WrapperMessage wrapper;
		if (wrapper.ParseFromArray(&c->data[4], expected_size) == false) {
			std::cout << "PARSING ERROR " << expected_size << "\n";
			c->data.erase(c->data.begin(), c->data.begin() + 4 + expected_size);
			continue;
		}
		c->data.erase(c->data.begin(), c->data.begin() + 4 + expected_size);

		// If backend is slow and it is sending us lot of message, there is possibility
		// that we don't receive PONG response before timeout. However, if we received
		// at least some data, it means backend is not dead and we can treat it as
		// PONG received event.
		if (c->pongReceived == false) {
			c->pongReceived = true;
		}

		// Handle payload in wrapper message
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
				handlePongReceived(c);
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
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED:
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING:
				handleChatStatePayload(wrapper.payload(), wrapper.type());
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
			case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_REMOVED:
				handleBuddyRemovedPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_QUERY:
				handleQueryPayload(c, wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_BACKEND_CONFIG:
				handleBackendConfigPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_ROOM_LIST:
				handleRoomListPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE_ACK:
				handleConvMessageAckPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_RAW_XML:
				handleRawXML(wrapper.payload());
				break;
			default:
				return;
		}
	}
}

void NetworkPluginServer::send(std::shared_ptr<Swift::Connection> &c, const std::string &data) {
	// generate header - size of wrapper message
	uint32_t size = htonl(data.size());
	char *header = (char *) &size;

	// send header together with wrapper message
	c->write(Swift::createSafeByteArray(std::string(header, 4) + data));
}

void NetworkPluginServer::sendToUserBackend(User *user, pbnetwork::WrapperMessage_Type type, const std::string &payload) {
	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}

	std::string message = payload;
	WRAP(message, type);
	send(c->connection, message);
}

void NetworkPluginServer::pingTimeout() {
	LOG4CXX_INFO(networkPluginServerLogger, "Sending PING to backends");
	// TODO: move to separate timer, those 2 loops could be expensive
	// Some users are connected for weeks and they are blocking backend to be destroyed and its memory
	// to be freed. We are finding users who are inactive for more than "idle_reconnect_time" seconds and
	// reconnect them to long-running backend, where they can idle hapilly till the end of ages.
	time_t now = time(NULL);
	std::vector<User *> usersToMove;
	long diff = CONFIG_INT(m_config, "service.idle_reconnect_time");
	if (diff != 0) {
		for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
			// Users from long-running backends can't be moved
			if ((*it)->longRun) {
				continue;
			}

			// Find users which are inactive for more than 'diff'
			for (User *u : (*it)->users) {
				if (now - u->getLastActivity() > diff) {
					usersToMove.push_back(u);
				}
			}
		}

		// Move inactive users to long-running backend.
		for (User *u : usersToMove) {
			LOG4CXX_INFO(networkPluginServerLogger, "Moving user " << u->getUserName() << " to long-running backend");
			if (!moveToLongRunBackend(u))
				break;
		}
	}

	// We have to remove startingBackend flag otherwise 1 broken backend start could
	// block the backend.
	m_startingBackend = false;

	// check ping responses
	std::vector<Backend *> toRemove;
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		// pong has been received OR backend just connected and did not have time to answer the ping
		// request.
		if ((*it)->pongReceived || (*it)->pongReceived == -1) {
			// Don't send another ping if pongReceived == -1, because we've already sent one
			// when registering backend.
			if ((*it)->pongReceived) {
				sendPing((*it));
			}
			else {
				LOG4CXX_INFO(networkPluginServerLogger, "Tried to send PING to backend without pongReceived= " << (*it)->pongReceived << ": (ID=" << (*it)->id << ")");
			}
		}
		else {
			LOG4CXX_INFO(networkPluginServerLogger, "Disconnecting backend " << (*it) << " (ID=" << (*it)->id << "). PING response not received.");
			toRemove.push_back(*it);

#ifndef WIN32
			// generate coredump for this backend to find out why it wasn't able to respond to PING
			std::string pid = (*it)->id;
			if (!pid.empty()) {
				try {
					kill(std::stoi(pid), SIGABRT);
				}
				catch (...) { }
			}
#endif
		}

		if ((*it)->users.size() == 0) {
			LOG4CXX_INFO(networkPluginServerLogger, "Disconnecting backend " << (*it) << " (ID=" << (*it)->id << "). There are no users.");
			toRemove.push_back(*it);
		}
	}

	for (Backend *b : toRemove) {
		handleSessionFinished(b);
	}

	m_pingTimer->start();
}

void NetworkPluginServer::collectBackend() {
	// Stop accepting new users to backend with the biggest memory usage. This prevents backends
	// which are leaking to eat whole memory by connectin new users to legacy network.
	LOG4CXX_INFO(networkPluginServerLogger, "Collect backend called, finding backend which will be set to die");
	unsigned long max = 0;
	Backend *backend = NULL;
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->res > max) {
			max = (*it)->res;
			backend = (*it);
		}
	}

	if (backend) {
		if (m_collectTimer) {
			m_collectTimer->start();
		}
		LOG4CXX_INFO(networkPluginServerLogger, "Backend " << backend << " (ID=" << backend->id << ") is set to die");
		backend->acceptUsers = false;
	}
}

bool NetworkPluginServer::moveToLongRunBackend(User *user) {
	// Check if user has already some backend
	Backend *old = (Backend *) user->getData();
	if (!old) {
		LOG4CXX_INFO(networkPluginServerLogger, "User " << user->getUserName() << " does not have old backend. Not moving.");
		return true;
	}

	// if he's already on long run, do nothing
	if (old->longRun) {
		LOG4CXX_INFO(networkPluginServerLogger, "User " << user->getUserName() << " is already on long-running backend. Not moving.");
		return true;
	}

	// Get free longrun backend, if there's no longrun backend, create one and wait
	// for its connection
	Backend *backend = getFreeClient(false, true);
	if (!backend) {
		LOG4CXX_INFO(networkPluginServerLogger, "No free long-running backend for user " << user->getUserName() << ". Will try later");
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
	// Get free backend to handle this user or spawn new one if there's no free one.
	Backend *c = getFreeClient();

	// Add user to queue if there's no free backend to handle him so far.
	if (!c) {
		LOG4CXX_INFO(networkPluginServerLogger, "There is no backend to handle user " << user->getUserName() << ". Adding him to queue.");
		m_waitingUsers.push_back(user);
		return;
	}

	// Associate users with backend
	user->setData(c);
	c->users.push_back(user);

	// Don't forget to disconnect these in handleUserDestroyed!!!
	user->onReadyToConnect = [this, user]() { handleUserReadyToConnect(user); };
	user->onStatusChanged = [this, user](const pbnetwork::Status &status) { handleUserStatusChanged(user, status); };
	user->onRoomJoined = [this, user](const std::string &room, const std::string &nickname, const std::string &password) { handleRoomJoined(user, room, nickname, password); };
	user->onRoomLeft = [this, user](const std::string &room) { handleRoomLeft(user, room); };

	user->getRosterManager()->onBuddyAdded = [this, user](Buddy *buddy) { handleUserBuddyAdded(user, buddy); };
	user->getRosterManager()->onBuddyRemoved = [this, user](Buddy *buddy) { handleUserBuddyRemoved(user, buddy); };
}

void NetworkPluginServer::handleUserReadyToConnect(User *user) {
	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Login login;
	login.set_user(user->getUserName());
	login.set_legacyname(userInfo.uin);
	login.set_password(userInfo.password);
	pbnetwork::Login_UserSettings *settings = login.mutable_settings();
	auto value = settings->mutable_value();
	for (const auto& item : userInfo.settings) {
		(*value)[item.first] = item.second;
	}

	std::string message;
	login.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_LOGIN);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);

	// Send buddies
	if (CONFIG_BOOL_DEFAULTED(m_config, "features.send_buddies_on_login", false)) {
		pbnetwork::Buddies buddies;

		const RosterManager::BuddiesMap &roster = user->getRosterManager()->getBuddies();
		for (RosterManager::BuddiesMap::const_iterator bt = roster.begin(); bt != roster.end(); bt++) {
			Buddy *b = (*bt).second;
			if (!b) {
				continue;
			}

			pbnetwork::Buddy *buddy = buddies.add_buddy();
			buddy->set_username(user->getUserName());
			buddy->set_buddyname(b->getName());
			buddy->set_alias(b->getAlias());
			buddy->set_iconhash(b->getIconHash());
			for (const std::string &g : b->getGroups()) {
				buddy->add_group(g);
			}
			buddy->set_status(pbnetwork::STATUS_NONE);
		}

		std::string msg;
		buddies.SerializeToString(&msg);

		WRAP(msg, pbnetwork::WrapperMessage_Type_TYPE_BUDDIES);
		send(c->connection, msg);
	}
}

void NetworkPluginServer::handleUserStatusChanged(User *user, const pbnetwork::Status &s) {
	if (s.status() == pbnetwork::STATUS_NONE)
		return;

	pbnetwork::Status status(s);
	status.set_username(user->getUserName());

	std::string message;
	status.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_STATUS_CHANGED, message);
}

void NetworkPluginServer::handleRoomJoined(User *user, const std::string &r, const std::string &nickname, const std::string &password) {
	pbnetwork::Room room;
	room.set_username(user->getUserName());
	room.set_nickname(nickname);
	room.set_room(r);
	room.set_password(password);

	std::string message;
	room.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_JOIN_ROOM, message);
}

void NetworkPluginServer::handleRoomLeft(User *user, const std::string &r) {
	pbnetwork::Room room;
	room.set_username(user->getUserName());
	room.set_nickname("");
	room.set_room(r);
	room.set_password("");

	std::string message;
	room.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_LEAVE_ROOM, message);
}

void NetworkPluginServer::handleUserDestroyed(User *user) {
	m_waitingUsers.remove(user);
	UserInfo userInfo = user->getUserInfo();

	user->onReadyToConnect = nullptr;
	user->onStatusChanged = nullptr;
	user->onRoomJoined = nullptr;
	user->onRoomLeft = nullptr;

	user->getRosterManager()->onBuddyAdded = nullptr;
	user->getRosterManager()->onBuddyRemoved = nullptr;

	pbnetwork::Logout logout;
	logout.set_user(user->getUserName());
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

	// If backend should handle only one user, it must not accept another one before
	// we kill it, so set up willDie to true
	if (c->users.size() == 0 && CONFIG_INT(m_config, "service.users_per_backend") == 1) {
		LOG4CXX_INFO(networkPluginServerLogger, "Backend " << c->id << " will die, because the last user disconnected");
		c->willDie = true;
	}
}

void NetworkPluginServer::handleMessageReceived(Conversation *conv, pbnetwork::WrapperMessage_Type type, const pbnetwork::ConversationMessage &msg) {
	User *user = conv->getConversationManager()->getUser();
	user->updateLastActivity();

	switch (type) {
		case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPING:
		case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_TYPED:
		case pbnetwork::WrapperMessage_Type_TYPE_BUDDY_STOPPED_TYPING: {
			pbnetwork::Buddy buddy;
			buddy.set_username(user->getUserName());
			buddy.set_buddyname(conv->getLegacyName());

			std::string message;
			buddy.SerializeToString(&message);

			sendToUserBackend(user, type, message);
			break;
		}
		case pbnetwork::WrapperMessage_Type_TYPE_ATTENTION:
		case pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED: {
			pbnetwork::ConversationMessage m;
			m.set_username(user->getUserName());
			m.set_buddyname(conv->getLegacyName());
			m.set_message(msg.message());

			std::string message;
			m.SerializeToString(&message);

			sendToUserBackend(user, type, message);
			break;
		}
		case pbnetwork::WrapperMessage_Type_TYPE_CONV_MESSAGE: {
			// Send normal message
			if (msg.message().empty() && msg.xhtml().empty()) {
				break;
			}

			pbnetwork::ConversationMessage m;
			m.set_username(user->getUserName());
			m.set_buddyname(conv->getLegacyName());
			m.set_message(msg.message());
			m.set_xhtml(msg.xhtml());
			if (!msg.id().empty()) {
				m.set_id(msg.id());
			}

			std::string message;
			m.SerializeToString(&message);

			sendToUserBackend(user, type, message);
			break;
		}
		default:
			break;
	}
}

void NetworkPluginServer::handleBuddyRemoved(Buddy *b) {
	User *user = b->getRosterManager()->getUser();

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getUserName());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	for (const std::string &g : b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);

	std::string message;
	buddy.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_REMOVED, message);
}

void NetworkPluginServer::handleBuddyUpdated(Buddy *b, const pbnetwork::Buddy &item) {
	User *user = b->getRosterManager()->getUser();

	std::vector<std::string> groups(item.group().begin(), item.group().end());
	dynamic_cast<LocalBuddy *>(b)->setAlias(item.alias());
	dynamic_cast<LocalBuddy *>(b)->setGroups(groups);

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getUserName());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	for (const std::string &g : b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);

	std::string message;
	buddy.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED, message);
}

void NetworkPluginServer::handleBuddyAdded(Buddy *buddy, const pbnetwork::Buddy &item) {
	handleBuddyUpdated(buddy, item);
}

void NetworkPluginServer::handleUserBuddyAdded(User *user, Buddy *b) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user->getUserName());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	for (const std::string &g : b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);

	std::string message;
	buddy.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED, message);
}

void NetworkPluginServer::handleUserBuddyRemoved(User *user, Buddy *b) {
	handleBuddyRemoved(b);
}

void NetworkPluginServer::handleBlockToggled(Buddy *b) {
	User *user = b->getRosterManager()->getUser();

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getUserName());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	for (const std::string &g : b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);
	buddy.set_blocked(!b->isBlocked());

	std::string message;
	buddy.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED, message);
}


void NetworkPluginServer::handleVCardUpdated(User *user, const pbnetwork::VCard &v) {
	pbnetwork::VCard vcard;
	vcard.set_username(user->getUserName());
	vcard.set_buddyname("");
	vcard.set_id(0);
	vcard.set_photo(v.photo());
	vcard.set_nickname(v.nickname());

	std::string message;
	vcard.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_VCARD, message);
}

void NetworkPluginServer::handleVCardRequired(User *user, const std::string &name, unsigned int id) {
	pbnetwork::VCard vcard;
	vcard.set_username(user->getUserName());
	vcard.set_buddyname(name);
	vcard.set_id(id);

	std::string message;
	vcard.SerializeToString(&message);

	sendToUserBackend(user, pbnetwork::WrapperMessage_Type_TYPE_VCARD, message);
}

void NetworkPluginServer::sendPing(Backend *c) {

	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_PING);
	wrap.SerializeToString(&message);

	if (c->connection) {
		LOG4CXX_INFO(networkPluginServerLogger, "PING to " << c << " (ID=" << c->id << ")");
		send(c->connection, message);
		c->pongReceived = false;
	}
	else {
		LOG4CXX_WARN(networkPluginServerLogger, "Tried to send PING to backend without connection: " << c << " (ID=" << c->id << ")");
	}
// 	LOG4CXX_INFO(networkPluginServerLogger, "PING to " << c);
}

void NetworkPluginServer::sendAPIVersion(Backend *c) {

	pbnetwork::APIVersion apiver;
	apiver.set_version(NETWORK_PLUGIN_API_VERSION);

	std::string message;
	apiver.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_API_VERSION);

	if (c->connection) {
		LOG4CXX_INFO(networkPluginServerLogger, "API Version to " << c << " (ID=" << c->id << ")");
		send(c->connection, message);
	}
}

void NetworkPluginServer::handlePIDTerminated(unsigned long pid) {
	std::vector<unsigned long>::iterator log_id_it;
	log_id_it = std::find(m_pids.begin(), m_pids.end(), pid);
	if (log_id_it != m_pids.end()) {
		*log_id_it = 0;
	}
}

#ifndef _WIN32

static int sig_block_count = 0;
static sigset_t block_mask;

static void __block_signals ( void )
{
  static int init_done = 0;

  if ( (sig_block_count++) != 1 ) return;

  if ( init_done == 0 ) {
    sigemptyset ( &block_mask );
    sigaddset ( &block_mask, SIGPIPE );
    sigaddset ( &block_mask, SIGHUP );
    sigaddset ( &block_mask, SIGINT );
    sigaddset ( &block_mask, SIGQUIT );
    sigaddset ( &block_mask, SIGTERM );
    sigaddset ( &block_mask, SIGABRT );
    sigaddset ( &block_mask, SIGCHLD );
    init_done = 1;
  }

  sigprocmask ( SIG_BLOCK, &block_mask, NULL );
  return;
}

static void __unblock_signals ( void )
{
  sigset_t sigset;

  if ( (sig_block_count--) != 0 ) return;
  sigprocmask ( SIG_UNBLOCK, &block_mask, NULL );

  if ( sigpending ( &sigset ) == 0 ) {
    if ( sigismember ( &sigset, SIGCHLD ) ) {
      raise ( SIGCHLD );
    }
  }
}

#endif

NetworkPluginServer::Backend *NetworkPluginServer::getFreeClient(bool acceptUsers, bool longRun, bool check) {
	NetworkPluginServer::Backend *c = NULL;

	long diff = CONFIG_INT(m_config, "service.login_delay");
	time_t now = time(NULL);
	if (diff && (now - m_lastLogin < diff)) {
		m_loginTimer->stop();
		m_loginTimer = m_component->getNetworkFactories()->getTimerFactory()->createTimer((diff - (now - m_lastLogin)) * 1000);
		m_loginTimer->onTick.connect(boost::bind(&NetworkPluginServer::loginDelayFinished, this));
		m_loginTimer->start();
		LOG4CXX_INFO(networkPluginServerLogger, "Postponing login because of service.login_delay setting");
		return NULL;
	}

	if (!check) {
		m_lastLogin = time(NULL);
	}

	// Check all backends and find free one
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->willDie == false && (*it)->acceptUsers == acceptUsers && (int) (*it)->users.size() < CONFIG_INT(m_config, "service.users_per_backend") && (*it)->connection && (*it)->longRun == longRun) {
			c = *it;
			// if we're not reusing all backends and backend is full, stop accepting new users on this backend
			if (!CONFIG_BOOL(m_config, "service.reuse_old_backends")) {
				if (!check && (int) c->users.size() + 1 >= CONFIG_INT(m_config, "service.users_per_backend")) {
					c->acceptUsers = false;
				}
			}
			break;
		}
	}

	// there's no free backend, so spawn one.
	if (c == NULL && !m_startingBackend) {
		m_isNextLongRun = longRun;
		m_startingBackend = true;

#ifndef _WIN32
		__block_signals();
#endif
		std::vector<unsigned long>::iterator log_id_it;
		log_id_it = std::find(m_pids.begin(), m_pids.end(), 0);
		std::string log_id = "";
		if (log_id_it == m_pids.end()) {
			log_id = std::to_string(m_pids.size() + 1);
		}
		else {
			log_id = std::to_string(log_id_it - m_pids.begin() + 1);
		}
		unsigned long pid = exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), log_id.c_str(), m_config->getCommandLineArgs().c_str());
		if (log_id_it == m_pids.end()) {
			m_pids.push_back(pid);
		}
		else {
			*log_id_it = pid;
		}
#ifndef _WIN32
		__unblock_signals();
#endif
	}

	return c;
}

}
