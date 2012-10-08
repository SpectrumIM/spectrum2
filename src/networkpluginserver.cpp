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
#include "transport/memoryreadbytestream.h"
#include "transport/logging.h"
#include "transport/admininterface.h"
#include "blockresponder.h"
#include "Swiften/Swiften.h"
#include "Swiften/Server/ServerStanzaChannel.h"
#include "Swiften/Elements/StreamError.h"
#include "Swiften/Network/BoostConnectionServer.h"
#include "Swiften/Elements/AttentionPayload.h"
#include "Swiften/Elements/XHTMLIMPayload.h"
#include "Swiften/Elements/InvisiblePayload.h"
#include "Swiften/Elements/SpectrumErrorPayload.h"
#include "transport/protocol.pb.h"
#include "transport/util.h"

#include "utf8.h"

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

class NetworkConversation : public Conversation {
	public:
		NetworkConversation(ConversationManager *conversationManager, const std::string &legacyName, bool muc = false) : Conversation(conversationManager, legacyName, muc) {
		}

		// Called when there's new message to legacy network from XMPP network
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
static unsigned long exec_(const std::string& exePath, const char *host, const char *port, const char *cmdlineArgs) {
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
	finalExePath += std::string(" --host ") + host + " --port " + port + " " + cmdlineArgs;
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
		std::string group = payload.group(i);
		utf8::replace_invalid(payload.group(i).begin(), payload.group(i).end(), group.begin(), '_');
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
	m_ftManager = ftManager;
	m_userManager = userManager;
	m_config = config;
	m_component = component;
	m_isNextLongRun = false;
	m_adminInterface = NULL;
	m_startingBackend = false;
	m_component->m_factory = new NetworkFactory(this);
	m_userManager->onUserCreated.connect(boost::bind(&NetworkPluginServer::handleUserCreated, this, _1));
	m_userManager->onUserDestroyed.connect(boost::bind(&NetworkPluginServer::handleUserDestroyed, this, _1));

	m_pingTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(20000);
	m_pingTimer->onTick.connect(boost::bind(&NetworkPluginServer::pingTimeout, this));
	m_pingTimer->start();

	if (CONFIG_INT(m_config, "service.memory_collector_time") != 0) {
		m_collectTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(CONFIG_INT(m_config, "service.memory_collector_time"));
		m_collectTimer->onTick.connect(boost::bind(&NetworkPluginServer::collectBackend, this));
		m_collectTimer->start();
	}

	m_vcardResponder = new VCardResponder(component->getIQRouter(), component->getNetworkFactories(), userManager);
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

	m_server = component->getNetworkFactories()->getConnectionServerFactory()->createConnectionServer(Swift::HostAddress(CONFIG_STRING(m_config, "service.backend_host")), boost::lexical_cast<int>(CONFIG_STRING(m_config, "service.backend_port")));
	m_server->onNewConnection.connect(boost::bind(&NetworkPluginServer::handleNewClientConnection, this, _1));
	m_server->start();

	LOG4CXX_INFO(logger, "Listening on host " << CONFIG_STRING(m_config, "service.backend_host") << " port " << CONFIG_STRING(m_config, "service.backend_port"));

	while (true) {
		unsigned long pid = exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getCommandLineArgs().c_str());
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
					LOG4CXX_ERROR(logger, "Backend can not be started, exit_code=" << WEXITSTATUS(status) << ", possible error: " << strerror(WEXITSTATUS(status)));
					continue;
				}
			}
			else {
				LOG4CXX_ERROR(logger, "Backend can not be started");
				continue;
			}
		}

		signal(SIGCHLD, SigCatcher);
#endif
		// quit the while loop
		break;
	}

}

NetworkPluginServer::~NetworkPluginServer() {
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
	delete m_vcardResponder;
	delete m_rosterResponder;
	delete m_blockResponder;
}

void NetworkPluginServer::handleNewClientConnection(boost::shared_ptr<Swift::Connection> c) {
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
	std::string field = payload.fullname();

	boost::shared_ptr<Swift::VCard> vcard(new Swift::VCard());

	utf8::replace_invalid(payload.fullname().begin(), payload.fullname().end(), field.begin(), '_');
	vcard->setFullName(field);

	field = payload.nickname();

	utf8::replace_invalid(payload.nickname().begin(), payload.nickname().end(), field.begin(), '_');
	vcard->setNickname(field);

	vcard->setPhoto(Swift::createByteArray(payload.photo()));

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

	// Create subscribe presence and forward it to XMPP side
	Swift::Presence::ref response = Swift::Presence::create();
	response->setTo(user->getJID());
	std::string name = payload.buddyname();

	name = Swift::JID::getEscapedNode(name);

// 	if (name.find_last_of("@") != std::string::npos) { // OK when commented
// 		name.replace(name.find_last_of("@"), 1, "%"); // OK when commented
// 	}

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

	// We're not creating new Conversation just because of chatstates.
	// Some networks/clients spams with chatstates a lot and it leads to bigger memory usage.
	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		return;
	}

	// Forward chatstate
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
		std::vector<std::string> groups;
		for (int i = 0; i < payload.group_size(); i++) {
			groups.push_back(payload.group(i));
		}
		buddy = new LocalBuddy(user->getRosterManager(), -1, payload.buddyname(), payload.alias(), groups, BUDDY_JID_ESCAPING);
		if (!buddy->isValid()) {
			delete buddy;
			return;
		}
		buddy->setStatus(Swift::StatusShow((Swift::StatusShow::Type) payload.status()), payload.statusmessage());
		buddy->setIconHash(payload.iconhash());
		buddy->setBlocked(payload.blocked());
		user->getRosterManager()->setBuddy(buddy);
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

	conv->handleParticipantChanged(payload.nickname(), payload.flag(), payload.status(), payload.statusmessage(), payload.newname());
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

	if (payload.ParseFromString(data) == false) {
		// TODO: ERROR
		return;
	}

	User *user = m_userManager->getUser(payload.username());
	if (!user)
		return;

	// Message from legacy network triggers network acticity
	user->updateLastActivity();

	// Set proper body.
	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	if (subject) {
		msg->setSubject(payload.message());
	}
	else {
		msg->setBody(payload.message());
	}

	// Add xhtml-im payload.
	if (CONFIG_BOOL(m_config, "service.enable_xhtml") && !payload.xhtml().empty()) {
		msg->addPayload(boost::make_shared<Swift::XHTMLIMPayload>(payload.xhtml()));
	}

	// Create new Conversation if it does not exist
	NetworkConversation *conv = (NetworkConversation *) user->getConversationManager()->getConversation(payload.buddyname());
	if (!conv) {
		conv = new NetworkConversation(user->getConversationManager(), payload.buddyname());
		user->getConversationManager()->addConversation(conv);
		conv->onMessageToSend.connect(boost::bind(&NetworkPluginServer::handleMessageReceived, this, _1, _2));
	}

	// Forward it
	conv->handleMessage(msg, payload.nickname());
	m_userManager->messageToXMPPSent();
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
	boost::shared_ptr<MemoryReadBytestream> bytestream(new MemoryReadBytestream(payload.size()));
	bytestream->onDataNeeded.connect(boost::bind(&NetworkPluginServer::handleFTDataNeeded, this, c, bytestream_id + 1));

	LOG4CXX_INFO(logger, "jid=" << buddy->getJID());

	FileTransferManager::Transfer transfer = m_ftManager->sendFile(user, buddy, bytestream, fileInfo);
	if (!transfer.ft) {
		handleFTRejected(user, payload.buddyname(), payload.filename(), payload.size());
		return;
	}

	m_filetransfers[++bytestream_id] = transfer;
	transfer.ft->onStateChange.connect(boost::bind(&NetworkPluginServer::handleFTStateChanged, this, _1, payload.username(), payload.buddyname(), payload.filename(), payload.size(), bytestream_id));
	transfer.ft->start();
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

void NetworkPluginServer::handlePongReceived(Backend *c) {
	// This could be first PONG from the backend
	if (c->pongReceived == -1) {
		// Backend is fully ready to handle requests
		c->willDie = false;

		if (m_clients.size() == 1) {
			// first backend connected, start the server, we're ready.
			m_component->start();
		}

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

	boost::shared_ptr<Swift::Message> msg(new Swift::Message());
	msg->setBody(payload.config());
	m_adminInterface->handleQuery(msg);

	pbnetwork::BackendConfig response;
	response.set_config(msg->getBody());

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

void NetworkPluginServer::handleDataRead(Backend *c, boost::shared_ptr<Swift::SafeByteArray> data) {
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
			default:
				return;
		}
	}
}

void NetworkPluginServer::send(boost::shared_ptr<Swift::Connection> &c, const std::string &data) {
	// generate header - size of wrapper message
	uint32_t size = htonl(data.size());
	char *header = (char *) &size;

	// send header together with wrapper message
	c->write(Swift::createSafeByteArray(std::string(header, 4) + data));
}

void NetworkPluginServer::pingTimeout() {
	// TODO: move to separate timer, those 2 loops could be expensive
	// Some users are connected for weeks and they are blocking backend to be destroyed and its memory
	// to be freed. We are finding users who are inactive for more than "idle_reconnect_time" seconds and
	// reconnect them to long-running backend, where they can idle hapilly till the end of ages.
	time_t now = time(NULL);
	std::vector<User *> usersToMove;
	unsigned long diff = CONFIG_INT(m_config, "service.idle_reconnect_time");
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
	user->onRoomJoined.connect(boost::bind(&NetworkPluginServer::handleRoomJoined, this, user, _1, _2, _3, _4));
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
	user->onRoomJoined.disconnect(boost::bind(&NetworkPluginServer::handleRoomJoined, this, user, _1, _2, _3, _4));
	user->onRoomLeft.disconnect(boost::bind(&NetworkPluginServer::handleRoomLeft, this, user, _1));

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


void NetworkPluginServer::handleVCardUpdated(User *user, boost::shared_ptr<Swift::VCard> v) {
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
	if (state.state == Swift::FileTransfer::State::Transferring) {
		handleFTAccepted(user, buddyName, fileName, size, id);
	}
	else if (state.state == Swift::FileTransfer::State::Canceled) {
		handleFTRejected(user, buddyName, fileName, size);
	}
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
// 	LOG4CXX_INFO(logger, "PING to " << c);
}

NetworkPluginServer::Backend *NetworkPluginServer::getFreeClient(bool acceptUsers, bool longRun) {
	NetworkPluginServer::Backend *c = NULL;

	// Check all backends and find free one
	for (std::list<Backend *>::const_iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		if ((*it)->willDie == false && (*it)->acceptUsers == acceptUsers && (*it)->users.size() < CONFIG_INT(m_config, "service.users_per_backend") && (*it)->connection && (*it)->longRun == longRun) {
			c = *it;
			// if we're not reusing all backends and backend is full, stop accepting new users on this backend
			if (!CONFIG_BOOL(m_config, "service.reuse_old_backends")) {
				if (c->users.size() + 1 >= CONFIG_INT(m_config, "service.users_per_backend")) {
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
		exec_(CONFIG_STRING(m_config, "service.backend"), CONFIG_STRING(m_config, "service.backend_host").c_str(), CONFIG_STRING(m_config, "service.backend_port").c_str(), m_config->getCommandLineArgs().c_str());
	}

	return c;
}

}
