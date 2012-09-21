#include "ircnetworkplugin.h"
#include <IrcCommand>
#include <IrcMessage>

#define FROM_UTF8(WHAT) QString::fromUtf8((WHAT).c_str(), (WHAT).size())
#define TO_UTF8(WHAT) std::string((WHAT).toUtf8().data(), (WHAT).toUtf8().size())

IRCNetworkPlugin::IRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port) {
	this->config = config;
	m_socket = new QTcpSocket();
	m_socket->connectToHost(FROM_UTF8(host), port);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));
}

void IRCNetworkPlugin::readData() {
	size_t availableBytes = m_socket->bytesAvailable();
	if (availableBytes == 0)
		return;

	std::cout << "READ\n";
	std::string d = std::string(m_socket->readAll().data(), availableBytes);
	handleDataRead(d);
}

void IRCNetworkPlugin::sendData(const std::string &string) {
	m_socket->write(string.c_str(), string.size());
}

void IRCNetworkPlugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
	// Server is in server-mode, so user is JID of server when we want to connect
	if (CONFIG_BOOL(config, "service.server_mode")) {
		MyIrcSession *session = new MyIrcSession(user, this);
		std::string h = user.substr(0, user.find("@"));
		session->setNickName(FROM_UTF8(h.substr(0, h.find("%"))));
		session->setHost(FROM_UTF8(h.substr(h.find("%") + 1)));
		session->setPort(6667);
		session->open();
		std::cout << "CONNECTING IRC NETWORK " << h.substr(h.find("%") + 1) << "\n";
		m_sessions[user] = session;
	}
	else {
		handleConnected(user);
	}
}

void IRCNetworkPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
	if (m_sessions[user] == NULL)
		return;
	m_sessions[user]->close();
	m_sessions[user]->deleteLater();
	m_sessions.erase(user);
}

void IRCNetworkPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/) {
	std::string u = user;
	std::cout << "AAAAA " << legacyName << "\n";
	if (!CONFIG_BOOL(config, "service.server_mode")) {
		u = user + legacyName.substr(legacyName.find("@") + 1);
		if (u.find("/") != std::string::npos) {
			u = u.substr(0, u.find("/"));
		}
	}
	if (m_sessions[u] == NULL) {
		std::cout << "No session for " << u << "\n";
		return;
	}

	std::string r = legacyName;
	if (!CONFIG_BOOL(config, "service.server_mode")) {
		if (legacyName.find("/") == std::string::npos) {
			r = legacyName.substr(0, r.find("@"));
		}
		else {
			r = legacyName.substr(legacyName.find("/") + 1);
		}
	}
	std::cout << "MESSAGE " << u << " " << r << "\n";
	m_sessions[u]->sendCommand(IrcCommand::createMessage(FROM_UTF8(r), FROM_UTF8(message)));
	std::cout << "SENT\n";
}

void IRCNetworkPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
	std::cout << "JOIN\n";
	std::string r = room;
	std::string u = user;
	if (!CONFIG_BOOL(config, "service.server_mode")) {
		u = user + room.substr(room.find("@") + 1);
		r = room.substr(0, room.find("@"));
	}
	if (m_sessions[u] == NULL) {
		// in gateway mode we want to login this user to network according to legacyName
		if (room.find("@") != std::string::npos) {
			// suffix is %irc.freenode.net to let MyIrcSession return #room%irc.freenode.net
			MyIrcSession *session = new MyIrcSession(user, this, room.substr(room.find("@")));
			session->setNickName(FROM_UTF8(nickname));
			session->setHost(FROM_UTF8(room.substr(room.find("@") + 1)));
			session->setPort(6667);
			session->open();
			std::cout << "CONNECTING IRC NETWORK " << room.substr(room.find("@") + 1) << "\n";
			std::cout << "SUFFIX " << room.substr(room.find("@")) << "\n";
			m_sessions[u] = session;
		}
		else {
			return;
		}
	}
	std::cout << "JOINING " << r << "\n";
	m_sessions[u]->addAutoJoinChannel(r, password);
	m_sessions[u]->sendCommand(IrcCommand::createJoin(FROM_UTF8(r), FROM_UTF8(password)));
	m_sessions[u]->rooms += 1;
	// update nickname, because we have nickname per session, no nickname per room.
	handleRoomNicknameChanged(user, r, TO_UTF8(m_sessions[u]->nickName()));
}

void IRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::string r = room;
	std::string u = user;
	if (!CONFIG_BOOL(config, "service.server_mode")) {
		r = room.substr(0, room.find("@"));
		u = user + room.substr(room.find("@") + 1);
	}

	if (m_sessions[u] == NULL)
		return;

	m_sessions[u]->sendCommand(IrcCommand::createPart(FROM_UTF8(r)));
	m_sessions[u]->removeAutoJoinChannel(r);
	m_sessions[u]->rooms -= 1;

	if (m_sessions[u]->rooms <= 0) {
		m_sessions[u]->close();
		m_sessions[u]->deleteLater();
		m_sessions.erase(u);
	}
}
