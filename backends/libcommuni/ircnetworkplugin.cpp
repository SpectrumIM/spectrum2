#include "ircnetworkplugin.h"
#include <IrcCommand>
#include <IrcMessage>
#include "transport/logging.h"

DEFINE_LOGGER(logger, "IRCNetworkPlugin");

#define FROM_UTF8(WHAT) QString::fromUtf8((WHAT).c_str(), (WHAT).size())
#define TO_UTF8(WHAT) std::string((WHAT).toUtf8().data(), (WHAT).toUtf8().size())

IRCNetworkPlugin::IRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port) {
	this->config = config;
	m_currentServer = 0;
	m_socket = new QTcpSocket();
	m_socket->connectToHost(FROM_UTF8(host), port);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));

	std::string server = CONFIG_STRING_DEFAULTED(config, "service.irc_server", "");
	if (!server.empty()) {
		m_servers.push_back(server);
	}
	else {
		
		std::list<std::string> list;
		list = CONFIG_LIST_DEFAULTED(config, "service.irc_server", list);
		
		m_servers.insert(m_servers.begin(), list.begin(), list.end());
	}

	if (CONFIG_HAS_KEY(config, "service.irc_identify")) {
		m_identify = CONFIG_STRING(config, "service.irc_identify");
	}
	else {
		m_identify = "NickServ identify $name $password";
	}
}

void IRCNetworkPlugin::tryNextServer() {
	if (!m_servers.empty()) {
		int nextServer = (m_currentServer + 1) % m_servers.size();
		LOG4CXX_INFO(logger, "Server " << m_servers[m_currentServer] << " disconnected user. Next server to try will be " << m_servers[nextServer]);
		m_currentServer = nextServer;
	}
}

void IRCNetworkPlugin::readData() {
	size_t availableBytes = m_socket->bytesAvailable();
	if (availableBytes == 0)
		return;

	std::string d = std::string(m_socket->readAll().data(), availableBytes);
	handleDataRead(d);
}

void IRCNetworkPlugin::sendData(const std::string &string) {
	m_socket->write(string.c_str(), string.size());
}

MyIrcSession *IRCNetworkPlugin::createSession(const std::string &user, const std::string &hostname, const std::string &nickname, const std::string &password, const std::string &suffix) {
	MyIrcSession *session = new MyIrcSession(user, this, suffix);
	session->setUserName(FROM_UTF8(nickname));
	session->setNickName(FROM_UTF8(nickname));
	session->setRealName(FROM_UTF8(nickname));
	session->setHost(FROM_UTF8(hostname));
	session->setPort(6667);
	session->setEncoding( "utf-8" );

	if (!password.empty()) {
		std::string identify = m_identify;
		boost::replace_all(identify, "$password", password);
		boost::replace_all(identify, "$name", nickname);
		session->setIdentify(identify);
	}

	LOG4CXX_INFO(logger, user << ": Connecting " << hostname << " as " << nickname << ", suffix=" << suffix);

	session->open();

	return session;
}

void IRCNetworkPlugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
	if (!m_servers.empty()) {
		// legacy name is users nickname
		if (m_sessions[user] != NULL) {
			LOG4CXX_WARN(logger, user << ": Already logged in.");
			return;
		}

		m_sessions[user] = createSession(user, m_servers[m_currentServer], legacyName, password, "");
	}
	else {
		// We are waiting for first room join to connect user to IRC network, because we don't know which
		// network he choose...
		LOG4CXX_INFO(logger, user << ": Ready for connections");
		handleConnected(user);
	}
}

void IRCNetworkPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
	if (m_sessions[user] == NULL) {
		LOG4CXX_WARN(logger, user << ": Already disconnected.");
		return;
	}
	LOG4CXX_INFO(logger, user << ": Disconnecting.");
	m_sessions[user]->close();
	m_sessions[user]->deleteLater();
	m_sessions.erase(user);
}

std::string IRCNetworkPlugin::getSessionName(const std::string &user, const std::string &legacyName) {
	std::string u = user;
	if (!CONFIG_BOOL(config, "service.server_mode") && m_servers.empty()) {
		u = user + legacyName.substr(legacyName.find("@") + 1);
		if (u.find("/") != std::string::npos) {
			u = u.substr(0, u.find("/"));
		}
	}
	return u;
}

std::string IRCNetworkPlugin::getTargetName(const std::string &legacyName) {
	std::string r = legacyName;
// 	if (!CONFIG_BOOL(config, "service.server_mode")) {
		if (legacyName.find("/") == std::string::npos) {
			r = legacyName.substr(0, r.find("@"));
		}
		else {
			r = legacyName.substr(legacyName.find("/") + 1);
		}
// 	}
	return r;
}

void IRCNetworkPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/) {
	std::string session = getSessionName(user, legacyName);
	if (m_sessions[session] == NULL) {
		LOG4CXX_WARN(logger, user << ": Session name: " << session << ", No session for user");
		return;
	}

	std::string target = getTargetName(legacyName);

	LOG4CXX_INFO(logger, user << ": Session name: " << session << ", message to " << target);

	if (message.find("/me") == 0) {
		m_sessions[session]->sendCommand(IrcCommand::createCtcpAction(FROM_UTF8(target), FROM_UTF8(message.substr(4))));
	}
	else {
		m_sessions[session]->sendCommand(IrcCommand::createMessage(FROM_UTF8(target), FROM_UTF8(message)));
	}

	if (target.find("#") == 0) {
		handleMessage(user, legacyName, message, TO_UTF8(m_sessions[session]->nickName()));
	}
}

void IRCNetworkPlugin::handleRoomSubjectChangedRequest(const std::string &user, const std::string &room, const std::string &message) {
	std::string session = getSessionName(user, room);
	if (m_sessions[session] == NULL) {
		LOG4CXX_WARN(logger, user << ": Session name: " << session << ", No session for user");
		return;
	}

	std::string target = getTargetName(room);
	m_sessions[session]->sendCommand(IrcCommand::createTopic(FROM_UTF8(target), FROM_UTF8(message)));
}

void IRCNetworkPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
	std::string session = getSessionName(user, room);
	std::string target = getTargetName(room);

	LOG4CXX_INFO(logger, user << ": Session name: " << session << ", Joining room " << target);
	if (m_sessions[session] == NULL) {
		if (m_servers.empty()) {
			// in gateway mode we want to login this user to network according to legacyName
			if (room.find("@") != std::string::npos) {
				// suffix is %irc.freenode.net to let MyIrcSession return #room%irc.freenode.net
				m_sessions[session] = createSession(user, room.substr(room.find("@") + 1), nickname, "", room.substr(room.find("@")));
			}
			else {
				LOG4CXX_WARN(logger, user << ": There's no proper server defined in room to which this user wants to join: " << room);
				return;
			}
		}
		else {
			LOG4CXX_WARN(logger, user << ": Join room requested for unconnected user");
			return;
		}
	}

	m_sessions[session]->addAutoJoinChannel(target, password);
	m_sessions[session]->sendCommand(IrcCommand::createJoin(FROM_UTF8(target), FROM_UTF8(password)));
	m_sessions[session]->rooms += 1;
	// update nickname, because we have nickname per session, no nickname per room.
	handleRoomNicknameChanged(user, target, TO_UTF8(m_sessions[session]->nickName()));
}

void IRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::string session = getSessionName(user, room);
	std::string target = getTargetName(room);

	LOG4CXX_INFO(logger, user << ": Session name: " << session << ", Leaving room " << target);
	if (m_sessions[session] == NULL)
		return;

	m_sessions[session]->sendCommand(IrcCommand::createPart(FROM_UTF8(target)));
	m_sessions[session]->removeAutoJoinChannel(target);
	m_sessions[session]->rooms -= 1;

	if (m_sessions[session]->rooms <= 0 && m_servers.empty()) {
		LOG4CXX_INFO(logger, user << ": Session name: " << session << ", User is not in any room, disconnecting from network");
		m_sessions[session]->close();
		m_sessions[session]->deleteLater();
		m_sessions.erase(session);
	}
}
