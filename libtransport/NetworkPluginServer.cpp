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
#include "transport/User.h"
#include "transport/Transport.h"
#include "transport/RosterManager.h"
#include "transport/UserManager.h"
#include "transport/ConversationManager.h"
#include "transport/LocalBuddy.h"
#include "transport/Config.h"
#include "transport/Conversation.h"
#include "transport/MemoryReadBytestream.h"
#include "transport/Logging.h"
#include "transport/AdminInterface.h"
#include "transport/Frontend.h"
#include "transport/Factory.h"
#include "transport/UserRegistry.h"
#include "transport/protocol.pb.h"
#include "transport/Util.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Network/BoostConnectionServer.h"
#include "Swiften/Network/ConnectionServerFactory.h"
#include "Swiften/Elements/AttentionPayload.h"
#include "Swiften/Elements/XHTMLIMPayload.h"
#include "Swiften/Elements/Delay.h"
#include "Swiften/Elements/DeliveryReceipt.h"
#include "Swiften/Elements/DeliveryReceiptRequest.h"
#include "Swiften/Elements/InvisiblePayload.h"
#include "Swiften/Elements/SpectrumErrorPayload.h"
#include "Swiften/Elements/RawXMLPayload.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include "transport/utf8.h"

#include <Swiften/FileTransfer/ReadBytestream.h>
#include <Swiften/Elements/StreamInitiationFileInfo.h>

#ifdef _WIN32
#include "windows.h"
#include <stdint.h>
#else
#include "sys/wait.h"
#include "sys/signal.h"
#include <sys/types.h>
#include <signal.h>
#include "popt.h"
#endif

using namespace Transport::Util;

namespace Transport {

static unsigned long backend_id;
static unsigned long bytestream_id;

DEFINE_LOGGER(logger, "NetworkPluginServer");

static NetworkPluginServer *_server;

class NetworkConversation : public Conversation {
	public:
		NetworkConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false) : Conversation(conversationManager, legacyName, muc) {
		}

		// Called when there's new message to legacy network from XMPP network
		void sendMessage(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &message) {
			onMessageToSend(this, message);
		}

		SWIFTEN_SIGNAL_NAMESPACE::signal<void (NetworkConversation *, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &)> onMessageToSend;
};

class NetworkFactory : public Factory {
	public:
		NetworkFactory(NetworkPluginServer *nps) {
			m_nps = nps;
		}

		virtual ~NetworkFactory() {}

		// Creates new conversation (NetworkConversation in this case)
		Conversation *createConversation(ConversationManager *conversationManager, const std::string &legacyName, bool isMuc) {
			NetworkConversation *nc = new NetworkConversation(conversationManager, legacyName, isMuc);
			nc->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, m_nps, _1, _2));
			return nc;
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
};

// Wraps google protobuf payload into WrapperMessage and serialize it to string
#define WRAP(MESSAGE, TYPE) 	pbnetwork::WrapperMessage wrap; \
	wrap.set_type(TYPE); \
	wrap.set_payload(MESSAGE); \
	wrap.SerializeToString(&MESSAGE);

// Executes new backend
static unsigned long exec_(const std::string& exePath, const char *host, const char *port, const char *log_id, const char *cmdlineArgs) {
	// BACKEND_ID is replaced with unique ID. The ID is increasing for every backend.
	std::string finalExePath = boost::replace_all_copy(exePath, "BACKEND_ID", boost::lexical_cast<std::string>(backend_id++));	

#ifdef _WIN32
	// Add host and port.
	std::ostringstream fullCmdLine;
	fullCmdLine << "\"" << finalExePath << "\" --host " << host << " --port " << port;

	if (cmdlineArgs)
		fullCmdLine << " " << cmdlineArgs;

	LOG4CXX_INFO(logger, "Starting new backend " << fullCmdLine.str());

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
		LOG4CXX_ERROR(logger, "Could not start process");
	}

	return 0;
#else
	// Add host and port.
	finalExePath += std::string(" --host ") + host + " --port " + port + " --service.backend_id=" + log_id + " " + cmdlineArgs;
	LOG4CXX_INFO(logger, "Starting new backend " << finalExePath);

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
		// close all files
		int maxfd=sysconf(_SC_OPEN_MAX);
		for (int fd=3; fd<maxfd; fd++) {
			close(fd);
		}
		// child process
		errno = 0;
		int ret = execv(argv[0], argv);
		if (ret == -1) {
			exit(errno);
		}
		exit(0);
	} else if ( pid < 0 ) {
		LOG4CXX_ERROR(logger, "Fork failed");
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
// 					LOG4CXX_ERROR(logger, "Backend can not be started, exit_code=" << WEXITSTATUS(status));
				}
			}
			else {
// 				LOG4CXX_ERROR(logger, "Backend can not be started");
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

	buddy->setStatus(Swift::StatusShow((Swift::StatusShow::Type) payload.status()), payload.statusmessage());
	buddy->setIconHash(payload.iconhash());
	buddy->setBlocked(payload.blocked());
}

NetworkPluginServer::NetworkPluginServer(Component *component, Config *config, UserManager *userManager, FileTransferManager *ftManager) {
	_server = this;
	m_ftManager = ftManager;
	m_userManager = userManager;
	m_config = config;
	m_component = component;
	m_isNextLongRun = false;
	m_adminInterface = NULL;
	m_startingBackend = false;
	m_lastLogin = 0;
	m_firstPong = true;
	m_xmppParser = new Swift::XMPPParser(this, &m_collection, component->getNetworkFactories()->getXMLParserFactory());
	m_xmppParser->parse("<stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='localhost' version='1.0'>");
#if HAVE_SWIFTEN_3
	m_serializer = new Swift::XMPPSerializer(&m_collection2, Swift::ClientStreamType, false);
#else
	m_serializer = new Swift::XMPPSerializer(&m_collection2, Swift::ClientStreamType);
#endif
	m_component->m_factory = new NetworkFactory(this);
	m_userManager->onUserCreated.connect(boost::bind(&NetworkPluginServer::handleUserCreated, this, _1));
	m_userManager->onUserDestroyed.connect(boost::bind(&NetworkPluginServer::handleUserDestroyed, this, _1));

	m_component->onRawIQReceived.connect(boost::bind(&NetworkPluginServer::handleRawIQReceived, this, _1));

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

	m_component->getFrontend()->onVCardRequired.connect(boost::bind(&NetworkPluginServer::handleVCardRequired, this, _1, _2, _3));
	m_component->getFrontend()->onVCardUpdated.connect(boost::bind(&NetworkPluginServer::handleVCardUpdated, this, _1, _2));

	m_component->getFrontend()->onBuddyAdded.connect(boost::bind(&NetworkPluginServer::handleBuddyAdded, this, _1, _2));
	m_component->getFrontend()->onBuddyRemoved.connect(boost::bind(&NetworkPluginServer::handleBuddyRemoved, this, _1));
	m_component->getFrontend()->onBuddyUpdated.connect(boost::bind(&NetworkPluginServer::handleBuddyUpdated, this, _1, _2));

// // 	m_blockResponder = new BlockResponder(component->getIQRouter(), userManager);
// // 	m_blockResponder->onBlockToggled.connect(boost::bind(&NetworkPluginServer::handleBlockToggled, this, _1));
// // 	m_blockResponder->start();

	m_server = component->getNetworkFactories()->getConnectionServerFactory()->createConnectionServer(SWIFT_HOSTADDRESS(CONFIG_STRING_DEFAULTED(m_config, "service.backend_host", "127.0.0.1")), boost::lexical_cast<int>(CONFIG_STRING(m_config, "service.backend_port")));
	m_server->onNewConnection.connect(boost::bind(&NetworkPluginServer::handleNewClientConnection, this, _1));
}

NetworkPluginServer::~NetworkPluginServer() {
#ifndef _WIN32
	signal(SIGCHLD, SIG_IGN);
#endif

	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		LOG4CXX_INFO(logger, "Stopping backend " << *it);
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
	delete m_xmppParser;
// 	delete m_vcardResponder;
// 	delete m_rosterResponder;
// 	delete m_blockResponder;
}

void NetworkPluginServer::start() {
	m_server->start();

	LOG4CXX_INFO(logger, "Listening on host " << CONFIG_STRING(m_config, "service.backend_host") << " port " << CONFIG_STRING(m_config, "service.backend_port"));

	while (true) {
		unsigned long pid = exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), "1", m_config->getCommandLineArgs().c_str());
		LOG4CXX_INFO(logger, "Tried to spawn first backend with pid " << pid);
		LOG4CXX_INFO(logger, "Backend should now connect to Spectrum2 instance. Spectrum2 won't accept any connection before backend connects");

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
						LOG4CXX_ERROR(logger, "Backend can not be started, because it needs database to store data, but the database backend is not configured.");
					}
					else {
						LOG4CXX_ERROR(logger, "Backend can not be started, exit_code=" << WEXITSTATUS(status) << ", possible error: " << strerror(WEXITSTATUS(status)));
						if (WEXITSTATUS(status) == ENOENT) {
							LOG4CXX_ERROR(logger, "This usually means the path to backend executable defined in config file as '[service] backend=\"...\"' is wrong or the executable does not exists.");
						}
						
					}
					LOG4CXX_ERROR(logger, "Check backend log for more details");
					continue;
				}
			}
			else {
				LOG4CXX_ERROR(logger, "Backend can not be started");
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

void NetworkPluginServer::handleNewClientConnection(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> c) {
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

	LOG4CXX_INFO(logger, "New" + (client->longRun ? std::string(" long-running") : "") +  " backend " << client << " connected. Current backend count=" << (m_clients.size() + 1));

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
	LOG4CXX_INFO(logger, "Backend " << c << " (ID=" << c->id << ") disconnected. Current backend count=" << (m_clients.size() - 1));

	// This backend will do, so we can't reconnect users to it in User::handleDisconnected call
	c->willDie = true;

	// If there are users associated with this backend, it must have crashed, so print error output
	// and disconnect users
	if (!c->users.empty()) {
		m_crashedBackends.push_back(c->id);
	}

	for (std::list<User *>::const_iterator it = c->users.begin(); it != c->users.end(); it++) {
		LOG4CXX_ERROR(logger, "Backend " << c << " (ID=" << c->id << ") disconnected (probably crashed) with active user " << (*it)->getJID().toString());
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
		LOG4CXX_ERROR(logger, "Connected payload received for unknown user " << payload.user());
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

	m_component->m_userRegistry->onPasswordInvalid(payload.user(), payload.message());

	User *user = m_userManager->getUser(payload.user());
	if (!user) {
		return;
	}
	user->handleDisconnected(payload.message(), (Swift::SpectrumErrorPayload::Error) payload.error());
}

void NetworkPluginServer::handleVCardPayload(const std::string &data) {
	pbnetwork::VCard payload;
	if (payload.ParseFromString(data) == false) {
		std::cout << "PARSING ERROR\n";
		// TODO: ERROR
		return;
	}
	std::string field;

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::VCard> vcard(new Swift::VCard());

	utf8::replace_invalid(payload.fullname().begin(), payload.fullname().end(), std::back_inserter(field), '_');
	vcard->setFullName(field);

	field.clear();

	utf8::replace_invalid(payload.nickname().begin(), payload.nickname().end(), std::back_inserter(field), '_');
	vcard->setNickname(field);

	vcard->setPhoto(Swift::createByteArray(payload.photo()));

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

	// Create subscribe presence and forward it to XMPP side
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(user->getJID());
	std::string name = payload.buddyname();

	if (CONFIG_BOOL_DEFAULTED(m_config, "service.jid_escaping", true)) {
		name = Swift::JID::getEscapedNode(name);
	}
	else {
		if (name.find_last_of("@") != std::string::npos) {
			name.replace(name.find_last_of("@"), 1, "%");
		}
	}

	response->setFrom(Swift::JID(name, m_component->getJID().toString()));
	response->setType(Swift::Presence::Subscribe);
	m_component->getFrontend()->sendPresence(response);
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

	// We're not creating new Conversation just because of chatstates.
	// Some networks/clients spams with chatstates a lot and it leads to bigger memory usage.
	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		return;
	}

	// Forward chatstate
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::ChatState>(type));

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
		buddy->setStatus(Swift::StatusShow((Swift::StatusShow::Type) payload.status()), payload.statusmessage());
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

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.room());
	if (!conv) {
		return;
	}

	conv->handleParticipantChanged(payload.nickname(), (Conversation::ParticipantFlag) payload.flag(), payload.status(), payload.statusmessage(), payload.newname(), payload.iconhash(), payload.alias());
}

void NetworkPluginServer::handleRoomChangedPayload(const std::string &data) {
	pbnetwork::Room payload;
	if (payload.ParseFromString(data) == false) {
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user) {
		LOG4CXX_ERROR(logger, "RoomChangePayload for unknown user " << user);
		return;
	}

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.room());
	if (!conv) {
		LOG4CXX_ERROR(logger, "RoomChangePayload for unknown conversation " << payload.room());
		return;
	}

	conv->setNickname(payload.nickname());
}

void NetworkPluginServer::handleConvMessagePayload(const std::string &data, bool subject) {
	pbnetwork::ConversationMessage payload;
	LOG4CXX_TRACE(logger, "handleConvMessagePayload");

	if (payload.ParseFromString(data) == false) {
		LOG4CXX_ERROR(logger, "handleConvMessagePayload: cannot parse payload");
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user) {
		LOG4CXX_ERROR(logger, "handleConvMessagePayload: unknown username " << payload.username());
		return;
	}

	// Message from legacy network triggers network acticity
	user->updateLastActivity();

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());

	// We can't create Conversation for payload with nickname, because this means the message is from room,
	// but this user is not in any room, so it's OK to just reject this message
	if (!conv && !payload.nickname().empty()) {
		LOG4CXX_WARN(logger, "handleConvMessagePayload: No conversation with name " << payload.buddyname());
		return;
	}

	if (conv && payload.pm()) {
		conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname() + "/" + payload.nickname());
		if (!conv) {
			conv = new NetworkConversation(user->getConversationManager(), payload.nickname());
			std::string name = payload.buddyname();
			conv->setRoom(name);
			conv->setNickname(payload.buddyname() + "/" + payload.nickname());

			user->getConversationManager()->addConversation(conv);
			conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
		}
	}

	// Create new Conversation if it does not exist
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
	}


	// Convert payload to Swift::Message
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
	if (subject) {
		msg->setSubject(payload.message());
	}
	else {
		msg->setBody(payload.message());
	}

	// Add xhtml-im payload.
	if (CONFIG_BOOL(m_config, "service.enable_xhtml") && !payload.xhtml().empty()) {
		msg->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::XHTMLIMPayload>(payload.xhtml()));
	}

	// Split the message if configured, or just preprocess
	LOG4CXX_TRACE(logger, "handleConvMessagePayload: wrapping media");
	typedef std::vector<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> > MsgList;
	MsgList msgs = wrapIncomingMedia(msg);

	// Forward all parts
	for (MsgList::iterator it=msgs.begin(); it!=msgs.end(); it++) {
		if (payload.headline()) {
			(*it)->setType(Swift::Message::Headline);
		}

		(*it)->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::ChatState>(Swift::ChatState::Active));

		if (!payload.timestamp().empty()) {
			boost::posix_time::ptime timestamp = boost::posix_time::from_iso_string(payload.timestamp());
			SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Delay> delay(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::Delay>());
			delay->setStamp(timestamp);
			(*it)->addPayload(delay);
		}

		conv->handleMessage((*it), payload.nickname(), payload.carbon());
	}

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
		LOG4CXX_WARN(logger, "Received message ack with empty ID, not forwarding to XMPP.");
		return;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::DeliveryReceipt>(payload.id()));

	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());

	// Receipts don't create conversation
	if (!conv) {
		return;
	}

	// Forward it
	conv->handleMessage(msg);
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

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(payload.message());
	msg->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::AttentionPayload>());

	// Attentions trigger new Conversation creation
	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
		user->getConversationManager()->addConversation(conv);
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
	c->id = payload.id();
}

void NetworkPluginServer::handleFTStartPayload(const std::string &data) {
	pbnetwork::File payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	LOG4CXX_INFO(logger, "handleFTStartPayload " << payload.filename() << " " << payload.buddyname());
	
	LocalBuddy *buddy = (LocalBuddy *) user->getRosterManager()->getBuddy(payload.buddyname());
	if (!buddy) {
		// TODO: escape? reject?
		return;
	}

	Swift::StreamInitiationFileInfo fileInfo;
	fileInfo.setSize(payload.size());
	fileInfo.setName(payload.filename());

	Backend *c = (Backend *) user->getData();
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<MemoryReadBytestream> bytestream(new MemoryReadBytestream(payload.size()));
	bytestream->onDataNeeded.connect(boost::bind(&NetworkPluginServer::handleFTDataNeeded, this, c, bytestream_id + 1));

	LOG4CXX_INFO(logger, "jid=" << buddy->getJID());

	FileTransferManager::Transfer transfer = m_ftManager->sendFile(user, buddy, bytestream, fileInfo);
	if (!transfer.ft) {
		handleFTRejected(user, payload.buddyname(), payload.filename(), payload.size());
		return;
	}

	m_filetransfers[++bytestream_id] = transfer;
#if !HAVE_SWIFTEN_3
	transfer.ft->onStateChange.connect(boost::bind(&NetworkPluginServer::handleFTStateChanged, this, _1, payload.username(), payload.buddyname(), payload.filename(), payload.size(), bytestream_id));
	transfer.ft->start();
#endif
}

void NetworkPluginServer::handleFTFinishPayload(const std::string &data) {
	pbnetwork::File payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	if (payload.has_ftid()) {
		if (m_filetransfers.find(payload.ftid()) != m_filetransfers.end()) {
			FileTransferManager::Transfer &transfer = m_filetransfers[payload.ftid()];
			transfer.ft->cancel();
		}
		else {
			LOG4CXX_ERROR(logger, "FTFinishPayload for unknown ftid=" << payload.ftid());
		}
	}

}

void NetworkPluginServer::handleFTDataPayload(Backend *b, const std::string &data) {
	pbnetwork::FileTransferData payload;
	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

// 	User *user = m_userManager->getUser(payload.username());
// 	if (!user)
// 		return;

	if (m_filetransfers.find(payload.ftid()) == m_filetransfers.end()) {
		LOG4CXX_ERROR(logger, "Uknown filetransfer with id " << payload.ftid());
		return;
	}

	FileTransferManager::Transfer &transfer = m_filetransfers[payload.ftid()];
	MemoryReadBytestream *bytestream = (MemoryReadBytestream *) transfer.readByteStream.get();

	if (bytestream->appendData(payload.data()) > 5000000) {
		pbnetwork::FileTransferData f;
		f.set_ftid(payload.ftid());
		f.set_data("");

		std::string message;
		f.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_PAUSE);

		send(b->connection, message);
	}
}

void NetworkPluginServer::handleFTDataNeeded(Backend *b, unsigned long ftid) {
	pbnetwork::FileTransferData f;
	f.set_ftid(ftid);
	f.set_data("");

	std::string message;
	f.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_CONTINUE);

	send(b->connection, message);
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

		LOG4CXX_INFO(logger, "Associating " << u->getJID().toString() << " with this backend");

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

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(payload.config());
	m_adminInterface->handleQuery(msg);

	pbnetwork::BackendConfig response;
#if HAVE_SWIFTEN_3
	response.set_config(msg->getBody().get_value_or(""));
#else
	response.set_config(msg->getBody());
#endif

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
			LOG4CXX_ERROR(logger, "Room list payload received for unknown user " << payload.user());
			return;
		}

		user->clearRoomList();
		for (int i = 0; i < payload.room_size() && i < payload.name_size(); i++) {
			user->addRoomToRoomList(Swift::JID::getEscapedNode(payload.room(i)) + "@" + m_component->getJID().toString(), payload.name(i));
		}
	}
	else {
		m_component->getFrontend()->clearRoomList();
		for (int i = 0; i < payload.room_size() && i < payload.name_size(); i++) {
			m_component->getFrontend()->addRoomToRoomList(Swift::JID::getEscapedNode(payload.room(i)) + "@" + m_component->getJID().toString(), payload.name(i));
		}
	}
}
#if HAVE_SWIFTEN_3
void NetworkPluginServer::handleElement(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ToplevelElement> element) {
#else
void NetworkPluginServer::handleElement(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Element> element) {
#endif
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Stanza> stanza = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::Stanza>(element);
	if (!stanza) {
		return;
	}

	User *user = m_userManager->getUser(stanza->getTo().toBare());
	if (!user)
		return;

	Swift::JID originalJID = stanza->getFrom();
	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(originalJID.toBare());

	LocalBuddy *buddy = (LocalBuddy *) user->getRosterManager()->getBuddy(stanza->getFrom().toBare());
	if (buddy) {
		const Swift::JID &jid = buddy->getJID();
		if (stanza->getFrom().getResource().empty()) {
			stanza->setFrom(Swift::JID(jid.getNode(), jid.getDomain()));
		}
		else {
			stanza->setFrom(Swift::JID(jid.getNode(), jid.getDomain(), stanza->getFrom().getResource()));
		}
	}
	else {
		std::string name = stanza->getFrom().toBare();
		if (conv && conv->isMUC()) {
			if (name.find_last_of("@") != std::string::npos) {
				name.replace(name.find_last_of("@"), 1, "%");
			}
		}
		else {
			if (CONFIG_BOOL_DEFAULTED(m_config, "service.jid_escaping", true)) {
				name = Swift::JID::getEscapedNode(name);
			}
			else {
				if (name.find_last_of("@") != std::string::npos) {
					name.replace(name.find_last_of("@"), 1, "%");
				}
			}
		}
		if (stanza->getFrom().getResource().empty()) {
			stanza->setFrom(Swift::JID(name, m_component->getJID().toString()));
		}
		else {
			stanza->setFrom(Swift::JID(name, m_component->getJID().toString(), stanza->getFrom().getResource()));
		}
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> message = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::Message>(stanza);
	if (message) {
		if (conv) {
			conv->handleRawMessage(message);
			return;
		}

		m_component->getFrontend()->sendMessage(message);
		return;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Presence> presence = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::Presence>(stanza);
	if (presence) {
		if (buddy) {
			if (!buddy->isAvailable() && presence->getType() != Swift::Presence::Unavailable) {
				buddy->m_status.setType(Swift::StatusShow::Online);
			}
			buddy->handleRawPresence(presence);
		}
		else if (conv) {
			conv->handleRawPresence(presence);
		}
		else {
			m_component->getFrontend()->sendPresence(presence);
		}

		return;
	}

	// TODO: Move m_id2resource in User and clean it up
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq = SWIFTEN_SHRPTR_NAMESPACE::dynamic_pointer_cast<Swift::IQ>(stanza);
	if (iq) {
		if (m_id2resource.find(stanza->getTo().toBare().toString() + stanza->getID()) != m_id2resource.end()) {
			iq->setTo(Swift::JID(iq->getTo().getNode(), iq->getTo().getDomain(), m_id2resource[stanza->getTo().toBare().toString() + stanza->getID()]));
			m_id2resource.erase(stanza->getTo().toBare().toString() + stanza->getID());
		}
		else {
			Swift::Presence::ref highest = m_component->getPresenceOracle()->getHighestPriorityPresence(user->getJID());
			if (highest) {
			    iq->setTo(highest->getFrom());
			} else {
			    iq->setTo(user->getJID());
			}
		}
		m_component->getFrontend()->sendIQ(iq);
		return;
	}
}

void NetworkPluginServer::handleRawXML(const std::string &xml) {
	m_xmppParser->parse(xml);
}

void NetworkPluginServer::handleRawPresenceReceived(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Presence> presence) {
	if (!CONFIG_BOOL_DEFAULTED(m_config, "features.rawxml", false)) {
		return;
	}

	User *user = m_userManager->getUser(presence->getFrom().toBare());
	if (!user)
		return;

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}

	Swift::JID legacyname = Swift::JID(Buddy::JIDToLegacyName(presence->getTo()));
	if (!presence->getTo().getResource().empty()) {
		presence->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain(), presence->getTo().getResource()));
	}
	else {
		presence->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain()));
	}

	std::string xml = safeByteArrayToString(m_serializer->serializeElement(presence));
	WRAP(xml, pbnetwork::WrapperMessage_Type_TYPE_RAW_XML);
	send(c->connection, xml);
}

void NetworkPluginServer::handleRawIQReceived(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::IQ> iq) {
	User *user = m_userManager->getUser(iq->getFrom().toBare());
	if (!user)
		return;

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}

	if (iq->getType() == Swift::IQ::Get) {
		m_id2resource[iq->getFrom().toBare().toString() + iq->getID()] = iq->getFrom().getResource();
	}

	Swift::JID legacyname = Swift::JID(Buddy::JIDToLegacyName(iq->getTo()));
	if (!iq->getTo().getResource().empty()) {
		iq->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain(), iq->getTo().getResource()));
	}
	else {
		iq->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain()));
	}

	std::string xml = safeByteArrayToString(m_serializer->serializeElement(iq));
	WRAP(xml, pbnetwork::WrapperMessage_Type_TYPE_RAW_XML);
	send(c->connection, xml);
}

void NetworkPluginServer::handleDataRead(Backend *c, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::SafeByteArray> data) {
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
			case pbnetwork::WrapperMessage_Type_TYPE_FT_START:
				handleFTStartPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_FT_FINISH:
				handleFTFinishPayload(wrapper.payload());
				break;
			case pbnetwork::WrapperMessage_Type_TYPE_FT_DATA:
				handleFTDataPayload(c, wrapper.payload());
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

void NetworkPluginServer::send(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Connection> &c, const std::string &data) {
	// generate header - size of wrapper message
	uint32_t size = htonl(data.size());
	char *header = (char *) &size;

	// send header together with wrapper message
	c->write(Swift::createSafeByteArray(std::string(header, 4) + data));
}

void NetworkPluginServer::pingTimeout() {
	LOG4CXX_INFO(logger, "Sending PING to backends");
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
			BOOST_FOREACH(User *u, (*it)->users) {
				if (now - u->getLastActivity() > diff) {
					usersToMove.push_back(u);
				}
			}
		}

		// Move inactive users to long-running backend.
		BOOST_FOREACH(User *u, usersToMove) {
			LOG4CXX_INFO(logger, "Moving user " << u->getJID().toString() << " to long-running backend");
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
				LOG4CXX_INFO(logger, "Tried to send PING to backend without pongReceived= " << (*it)->pongReceived << ": (ID=" << (*it)->id << ")");
			}
		}
		else {
			LOG4CXX_INFO(logger, "Disconnecting backend " << (*it) << " (ID=" << (*it)->id << "). PING response not received.");
			toRemove.push_back(*it);

#ifndef WIN32
			// generate coredump for this backend to find out why it wasn't able to respond to PING
			std::string pid = (*it)->id;
			if (!pid.empty()) {
				try {
					kill(boost::lexical_cast<int>(pid), SIGABRT);
				}
				catch (...) { }
			}
#endif
		}

		if ((*it)->users.size() == 0) {
			LOG4CXX_INFO(logger, "Disconnecting backend " << (*it) << " (ID=" << (*it)->id << "). There are no users.");
			toRemove.push_back(*it);
		}
	}

	BOOST_FOREACH(Backend *b, toRemove) {
		handleSessionFinished(b);
	}

	m_pingTimer->start();
}

void NetworkPluginServer::collectBackend() {
	// Stop accepting new users to backend with the biggest memory usage. This prevents backends
	// which are leaking to eat whole memory by connectin new users to legacy network.
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
		if (m_collectTimer) {
			m_collectTimer->start();
		}
		LOG4CXX_INFO(logger, "Backend " << backend << " (ID=" << backend->id << ") is set to die");
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
	// Get free backend to handle this user or spawn new one if there's no free one.
	Backend *c = getFreeClient();

	// Add user to queue if there's no free backend to handle him so far.
	if (!c) {
		LOG4CXX_INFO(logger, "There is no backend to handle user " << user->getJID().toString() << ". Adding him to queue.");
		m_waitingUsers.push_back(user);
		return;
	}

	// Associate users with backend
	user->setData(c);
	c->users.push_back(user);

	// Don't forget to disconnect these in handleUserDestroyed!!!
	user->onReadyToConnect.connect(boost::bind(&NetworkPluginServer::handleUserReadyToConnect, this, user));
	user->onPresenceChanged.connect(boost::bind(&NetworkPluginServer::handleUserPresenceChanged, this, user, _1));
	user->onRawPresenceReceived.connect(boost::bind(&NetworkPluginServer::handleRawPresenceReceived, this, _1));
	user->onRoomJoined.connect(boost::bind(&NetworkPluginServer::handleRoomJoined, this, user, _1, _2, _3, _4));
	user->onRoomLeft.connect(boost::bind(&NetworkPluginServer::handleRoomLeft, this, user, _1));

	user->getRosterManager()->onBuddyAdded.connect(boost::bind(&NetworkPluginServer::handleUserBuddyAdded, this, user, _1));
	user->getRosterManager()->onBuddyRemoved.connect(boost::bind(&NetworkPluginServer::handleUserBuddyRemoved, this, user, _1));
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
			buddy->set_username(user->getJID().toBare());
			buddy->set_buddyname(b->getName());
			buddy->set_alias(b->getAlias());
			buddy->set_iconhash(b->getIconHash());
			BOOST_FOREACH(const std::string &g, b->getGroups()) {
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

void NetworkPluginServer::handleUserPresenceChanged(User *user, Swift::Presence::ref presence) {
	if (presence->getShow() == Swift::StatusShow::None)
		return;

	handleRawPresenceReceived(presence);

	UserInfo userInfo = user->getUserInfo();

	pbnetwork::Status status;
	status.set_username(user->getJID().toBare());

	bool isInvisible = presence->getPayload<Swift::InvisiblePayload>() != NULL;
	if (isInvisible) {
		LOG4CXX_INFO(logger, "This presence is invisible");
		status.set_status((pbnetwork::STATUS_INVISIBLE));
	}
	else {
		status.set_status((pbnetwork::StatusType) presence->getShow());
	}

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

void NetworkPluginServer::handleRoomJoined(User *user, const Swift::JID &who, const std::string &r, const std::string &nickname, const std::string &password) {
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
}

void NetworkPluginServer::handleUserDestroyed(User *user) {
	m_waitingUsers.remove(user);
	UserInfo userInfo = user->getUserInfo();

	user->onReadyToConnect.disconnect(boost::bind(&NetworkPluginServer::handleUserReadyToConnect, this, user));
	user->onPresenceChanged.disconnect(boost::bind(&NetworkPluginServer::handleUserPresenceChanged, this, user, _1));
	user->onRawPresenceReceived.disconnect(boost::bind(&NetworkPluginServer::handleRawPresenceReceived, this, _1));
	user->onRoomJoined.disconnect(boost::bind(&NetworkPluginServer::handleRoomJoined, this, user, _1, _2, _3, _4));
	user->onRoomLeft.disconnect(boost::bind(&NetworkPluginServer::handleRoomLeft, this, user, _1));

	user->getRosterManager()->onBuddyAdded.disconnect(boost::bind(&NetworkPluginServer::handleUserBuddyAdded, this, user, _1));
	user->getRosterManager()->onBuddyRemoved.disconnect(boost::bind(&NetworkPluginServer::handleUserBuddyRemoved, this, user, _1));

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

	// If backend should handle only one user, it must not accept another one before 
	// we kill it, so set up willDie to true
	if (c->users.size() == 0 && CONFIG_INT(m_config, "service.users_per_backend") == 1) {
		LOG4CXX_INFO(logger, "Backend " << c->id << " will die, because the last user disconnected");
		c->willDie = true;
	}
}

void NetworkPluginServer::handleMessageReceived(NetworkConversation *conv, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> &msg) {
	conv->getConversationManager()->getUser()->updateLastActivity();

	if (CONFIG_BOOL_DEFAULTED(m_config, "features.rawxml", false)) {
		Backend *c = (Backend *) conv->getConversationManager()->getUser()->getData();
		if (!c) {
			return;
		}
		Swift::JID legacyname = Swift::JID(Buddy::JIDToLegacyName(msg->getTo()));
		if (!msg->getTo().getResource().empty()) {
			msg->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain(), msg->getTo().getResource()));
		}
		else {
			msg->setTo(Swift::JID(legacyname.getNode(), legacyname.getDomain()));
		}
		std::string xml = safeByteArrayToString(m_serializer->serializeElement(msg));
		WRAP(xml, pbnetwork::WrapperMessage_Type_TYPE_RAW_XML);
		send(c->connection, xml);
		return;
	}

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::ChatState> statePayload = msg->getPayload<Swift::ChatState>();
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

	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::AttentionPayload> attentionPayload = msg->getPayload<Swift::AttentionPayload>();
	if (attentionPayload) {
		pbnetwork::ConversationMessage m;
		m.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
		m.set_buddyname(conv->getLegacyName());
#if HAVE_SWIFTEN_3
		m.set_message(msg->getBody().get_value_or(""));
#else
		m.set_message(msg->getBody());
#endif

		std::string message;
		m.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_ATTENTION);

		Backend *c = (Backend *) conv->getConversationManager()->getUser()->getData();
		send(c->connection, message);
		return;
	}

	if (!msg->getSubject().empty()) {
		pbnetwork::ConversationMessage m;
		m.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
		m.set_buddyname(conv->getLegacyName());
		m.set_message(msg->getSubject());

		std::string message;
		m.SerializeToString(&message);

		WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_ROOM_SUBJECT_CHANGED);

		Backend *c = (Backend *) conv->getConversationManager()->getUser()->getData();
		send(c->connection, message);
		return;
	}
	

	std::string xhtml;
	SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::XHTMLIMPayload> xhtmlPayload = msg->getPayload<Swift::XHTMLIMPayload>();
	if (xhtmlPayload) {
		xhtml = xhtmlPayload->getBody();
	}

	// Send normal message
#if HAVE_SWIFTEN_3
	std::string body = msg->getBody().get_value_or("");
#else
	std::string body = msg->getBody();
#endif
	if (!body.empty() || !xhtml.empty()) {
		pbnetwork::ConversationMessage m;
		m.set_username(conv->getConversationManager()->getUser()->getJID().toBare());
		m.set_buddyname(conv->getLegacyName());
		m.set_message(body);
		m.set_xhtml(xhtml);
		SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::DeliveryReceiptRequest> receiptPayload = msg->getPayload<Swift::DeliveryReceiptRequest>();
		if (receiptPayload && !msg->getID().empty()) {
			m.set_id(msg->getID());
		}

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
	BOOST_FOREACH(const std::string &g, b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_REMOVED);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}


// Trims linebreaks and spaces from XHTML chunk of text
inline std::string xhtml_trim(std::string &s) {
	static const std::string t[] = {" ", "\n", "\t", "<br>", "<br/>"};
	bool match = true;
	while (match) {
		match = false;
		for (int i=0; i<sizeof(t)/sizeof(t[0]); i++)
			if (boost::algorithm::istarts_with(s, t[i])) {
				s.erase(0, t[i].size());
				match = true;
			}
	}
	match = true;
	while (match) {
		match = false;
		for (int i=0; i<sizeof(t)/sizeof(t[0]); i++)
			if (boost::algorithm::iends_with(s, t[i])) {
				s.erase(s.size() - t[i].size());
				match = true;
			}
	}
	return s;
}
//Same for plaintext
std::string plaintext_trim(std::string &text) {
	static std::string t = " \n\t";
	text.erase(text.find_last_not_of(t) + 1);
	text.erase(0, text.find_first_not_of(t));
	return text;
}

//Duplicates a Swift::Message and replaces its body and XHTML parts
Swift::Message::ref copySwiftMessage(const Swift::Message* msg, const std::string& xhtml, const std::string& body) {
    Swift::Message::ref this_msg(new Swift::Message());
    //Copy all payloads by reference, except for body and XHTML
    std::vector<Swift::Payload::ref> payloads = msg->getPayloads();
    for (size_t i=0; i<payloads.size(); i++) {
        if(!dynamic_cast<Swift::Body*>(payloads[i].get()) && !dynamic_cast<Swift::XHTMLIMPayload*>(payloads[i].get()))
            this_msg->addPayload(payloads[i]);
    }
    //Add new body and XHTML tags
    this_msg->setBody(body);
    this_msg->addPayload(SWIFTEN_SHRPTR_NAMESPACE::make_shared<Swift::XHTMLIMPayload>(xhtml));
    LOG4CXX_TRACE(logger, "Adding partial message: '" << xhtml << "', '" << body << "'");
    return this_msg;
}

/*
Process all media links (<img>, ..) in the message XHTML and apply required wrappers
and mitigations to ensure broader support for inplace display.
Returns a list of messages to deliver.

Copies any additional payloads into each of messages by reference (modify one == modify all).

Possible modes:
*/
enum OobMode {
	OobWrapAll	= 1,	//1. Wrap all media as OOB tags. (Standards-compliant)
	OobExclusive	= 2,	//2. Make the first media the only content of the message.
	OobSplit	= 3,	//3. Split into multiple text-only/media-only messages.
};

std::vector<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> >
NetworkPluginServer::wrapIncomingMedia(SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message>& msg) {
    std::vector<SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> > result;

    SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::XHTMLIMPayload> xhtmlPayload =
        msg->getPayload<Swift::XHTMLIMPayload>();
    if (!xhtmlPayload) { //XHTML not available or disabled
        result.push_back(msg);
        return result;
    }

    const std::string& xhtml = xhtmlPayload->getBody();
    if (xhtml.find("<img") == std::string::npos) { //Clearly no media
        result.push_back(msg);
        return result;
    }
    const std::string body = msg->getBody().get();

    OobMode oobMode = OobWrapAll;
    if (CONFIG_BOOL_DEFAULTED(m_config, "service.oob_split", false))
        //Split the message into parts so that each part only contains one media instance or one chunk of text
        oobMode = OobSplit;
    else if (CONFIG_BOOL_DEFAULTED(m_config, "service.oob_replace_body", false))
        // Some clients require plaintext to contain only the OOB URL to be displayed
        // This is not required by XEP and we lose parts of plaintext (e.g. captions).
        oobMode = OobExclusive;

    LOG4CXX_TRACE(logger, "wrapIncomingMedia: mode=" << (int) oobMode);
    LOG4CXX_TRACE(logger, "xhtml = " << xhtml);
    LOG4CXX_TRACE(logger, "body = " << body);


    //Find all <img...> entries
    //Handles: spaces, unrelated tags
    //Doesn't handle: unquoted src, escaped "'>s, quotes in quotes ("quote: 'text' end quote")
    static boost::regex image_expr("<img\\s+[^>]*src\\s*=\\s*[\"']([^\"']+)[\"'][^>]*>");

    bool matchCount = 0;
    std::string firstUrl;

    std::string::const_iterator xhtml_pos = xhtml.begin();
    std::string::size_type body_pos = 0;

    boost::smatch match;
    while(boost::regex_search(xhtml_pos, xhtml.end(), match, image_expr)) {
        matchCount++;
        const std::string& image_tag = match[0];
        const std::string& image_url = match[1];
        if (firstUrl.empty())
            firstUrl = image_url;
        LOG4CXX_TRACE(logger, "match: image_tag=" << image_tag << ", image_url="<< image_url);

        //Process the part before the match
        if ((oobMode == OobSplit) && (match[0].first != xhtml_pos)) {
            std::string xhtml_prev(xhtml_pos, match[0].first);
            xhtml_trim(xhtml_prev);

            //Find the same URI in the plaintext body
            std::string body_prev = "";
            std::string::size_type body_match = body.find(image_url, body_pos);
            if (body_match != std::string::npos) {
                body_prev = body.substr(body_pos, body_match-body_pos);
                plaintext_trim(body_prev);
                body_pos = body_match + image_url.size();
            }

            //If anything of that is not empty, post as a message
            if (!xhtml_prev.empty() || !body_prev.empty())
                result.push_back(copySwiftMessage(msg.get(), xhtml_prev, body_prev));
        }

        //Now the match
        SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::Message> this_msg;
        if (oobMode == OobSplit) {
            this_msg = copySwiftMessage(msg.get(), image_tag, image_url);
            result.push_back(this_msg);
        } else {
            this_msg = msg;
        }

        //Add OOB tag
        SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::RawXMLPayload>
            oob_payload(new Swift::RawXMLPayload(
                "<x xmlns='jabber:x:oob'><url>"
                + image_url
                + "</url>"
                + "</x>"
            ));
            // todo: add the payload itself as a caption
        this_msg->addPayload(oob_payload);

        //In single-OOB mode there's no point to process further media
        if (oobMode == OobExclusive)
            break;

        xhtml_pos = match[0].second;
    }

    //Post the text remainder
    if ((oobMode == OobSplit) && (xhtml_pos != xhtml.end())) {
        std::string xhtml_prev(xhtml_pos, xhtml.end());
        xhtml_trim(xhtml_prev);

        std::string body_prev = body.substr(body_pos, body.size()-body_pos);
        plaintext_trim(body_prev);

        if (!xhtml_prev.empty() || !body_prev.empty())
            result.push_back(copySwiftMessage(msg.get(), xhtml_prev, body_prev));
    }

    LOG4CXX_DEBUG(logger, "wrapIncomingMedia: matchCount==" << matchCount);

    if (oobMode != OobSplit)
        result.push_back(msg); //Push the non-split message only

    if (matchCount==0) {
        LOG4CXX_WARN(logger, "xhtml seems to contain an image, but doesn't match: " + xhtml);
    } else {
        // Replace the plaintext.
        // Normally it's up to the backend to provide us with <body> matching the <xhtml> version.
        if (oobMode == OobExclusive)
            msg->setBody(firstUrl);
    }
    return result;
}

void NetworkPluginServer::handleBuddyUpdated(Buddy *b, const Swift::RosterItemPayload &item) {
	User *user = b->getRosterManager()->getUser();

	dynamic_cast<LocalBuddy *>(b)->setAlias(item.getName());
	dynamic_cast<LocalBuddy *>(b)->setGroups(item.getGroups());

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getJID().toBare());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	BOOST_FOREACH(const std::string &g, b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);

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

void NetworkPluginServer::handleUserBuddyAdded(User *user, Buddy *b) {
	pbnetwork::Buddy buddy;
	buddy.set_username(user->getJID().toBare());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	BOOST_FOREACH(const std::string &g, b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::handleUserBuddyRemoved(User *user, Buddy *b) {
	handleBuddyRemoved(b);
}

void NetworkPluginServer::handleBlockToggled(Buddy *b) {
	User *user = b->getRosterManager()->getUser();

	pbnetwork::Buddy buddy;
	buddy.set_username(user->getJID().toBare());
	buddy.set_buddyname(b->getName());
	buddy.set_alias(b->getAlias());
	BOOST_FOREACH(const std::string &g, b->getGroups()) {
		buddy.add_group(g);
	}
	buddy.set_status(pbnetwork::STATUS_NONE);
	buddy.set_blocked(!b->isBlocked());

	std::string message;
	buddy.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_BUDDY_CHANGED);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}


void NetworkPluginServer::handleVCardUpdated(User *user, SWIFTEN_SHRPTR_NAMESPACE::shared_ptr<Swift::VCard> v) {
	if (!v) {
		LOG4CXX_INFO(logger, user->getJID().toString() << ": Received empty VCard");
		return;
	}

	pbnetwork::VCard vcard;
	vcard.set_username(user->getJID().toBare());
	vcard.set_buddyname("");
	vcard.set_id(0);
	vcard.set_photo(&v->getPhoto()[0], v->getPhoto().size());
	vcard.set_nickname(v->getNickname());

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

void NetworkPluginServer::handleFTAccepted(User *user, const std::string &buddyName, const std::string &fileName, unsigned long size, unsigned long ftID) {
	pbnetwork::File f;
	f.set_username(user->getJID().toBare());
	f.set_buddyname(buddyName);
	f.set_filename(fileName);
	f.set_size(size);
	f.set_ftid(ftID);

	std::string message;
	f.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_START);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::handleFTRejected(User *user, const std::string &buddyName, const std::string &fileName, unsigned long size) {
	pbnetwork::File f;
	f.set_username(user->getJID().toBare());
	f.set_buddyname(buddyName);
	f.set_filename(fileName);
	f.set_size(size);
	f.set_ftid(0);

	std::string message;
	f.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_FT_FINISH);

	Backend *c = (Backend *) user->getData();
	if (!c) {
		return;
	}
	send(c->connection, message);
}

void NetworkPluginServer::handleFTStateChanged(Swift::FileTransfer::State state, const std::string &userName, const std::string &buddyName, const std::string &fileName, unsigned long size, unsigned long id) {
	User *user = m_userManager->getUser(userName);
	if (!user) {
		// TODO: FIXME We have to remove filetransfer when use disconnects
		return;
	}
#if !HAVE_SWIFTEN_3
	if (state.state == Swift::FileTransfer::State::Transferring) {
		handleFTAccepted(user, buddyName, fileName, size, id);
	}
	else if (state.state == Swift::FileTransfer::State::Canceled) {
		handleFTRejected(user, buddyName, fileName, size);
	}
#endif
}

void NetworkPluginServer::sendPing(Backend *c) {

	std::string message;
	pbnetwork::WrapperMessage wrap;
	wrap.set_type(pbnetwork::WrapperMessage_Type_TYPE_PING);
	wrap.SerializeToString(&message);

	if (c->connection) {
		LOG4CXX_INFO(logger, "PING to " << c << " (ID=" << c->id << ")");
		send(c->connection, message);
		c->pongReceived = false;
	}
	else {
		LOG4CXX_WARN(logger, "Tried to send PING to backend without connection: " << c << " (ID=" << c->id << ")");
	}
// 	LOG4CXX_INFO(logger, "PING to " << c);
}

void NetworkPluginServer::sendAPIVersion(Backend *c) {

	pbnetwork::APIVersion apiver;
	apiver.set_version(NETWORK_PLUGIN_API_VERSION);

	std::string message;
	apiver.SerializeToString(&message);

	WRAP(message, pbnetwork::WrapperMessage_Type_TYPE_API_VERSION);

	if (c->connection) {
		LOG4CXX_INFO(logger, "API Version to " << c << " (ID=" << c->id << ")");
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
		LOG4CXX_INFO(logger, "Postponing login because of service.login_delay setting");
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
			log_id = boost::lexical_cast<std::string>(m_pids.size() + 1);
		}
		else {
			log_id = boost::lexical_cast<std::string>(log_id_it - m_pids.begin() + 1);
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
