#include "ircnetworkplugin.h"

IRCNetworkPlugin::IRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port) {
	this->config = config;
	m_socket = new QTcpSocket();
	m_socket->connectToHost(QString::fromStdString(host), port);
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
		session->setNick(QString::fromStdString(h.substr(0, h.find("%"))));
		session->connectToServer(QString::fromStdString(h.substr(h.find("%") + 1)), 6667);
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
	m_sessions[user]->disconnectFromServer();
	m_sessions[user]->deleteLater();
}

void IRCNetworkPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/) {
	if (m_sessions[user] == NULL)
		return;

	std::string r = legacyName;
	if (!CONFIG_BOOL(config, "service.server_mode")) {
		r = legacyName.substr(0, r.find("@"));
	}
	std::cout << "MESSAGE " << user << " " << r << "\n";
	m_sessions[user]->message(QString::fromStdString(r), QString::fromStdString(message));
	std::cout << "SENT\n";
}

void IRCNetworkPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
	std::cout << "JOIN\n";
	std::string r = room;
	if (m_sessions[user] == NULL) {
		// in gateway mode we want to login this user to network according to legacyName
		if (room.find("%") != std::string::npos) {
			// suffix is %irc.freenode.net to let MyIrcSession return #room%irc.freenode.net
			MyIrcSession *session = new MyIrcSession(user, this, room.substr(room.find("%")));
			session->setNick(QString::fromStdString(nickname));
			session->connectToServer(QString::fromStdString(room.substr(room.find("%") + 1)), 6667);
			std::cout << "CONNECTING IRC NETWORK " << room.substr(room.find("%") + 1) << "\n";
			std::cout << "SUFFIX " << room.substr(room.find("%")) << "\n";
			m_sessions[user] = session;
			r = room.substr(0, room.find("%"));
			std::cout << "room=" << r << "\n";
		}
		else {
			return;
		}
	}
	m_sessions[user]->addAutoJoinChannel(QString::fromStdString(r));
	m_sessions[user]->join(QString::fromStdString(r), QString::fromStdString(password));
	// update nickname, because we have nickname per session, no nickname per room.
	handleRoomNicknameChanged(user, r, m_sessions[user]->nick().toStdString());
}

void IRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::cout << "PART\n";
	if (m_sessions[user] == NULL)
		return;

	std::string r = room;
	if (!CONFIG_BOOL(config, "service.server_mode")) {
		r = room.substr(0, room.find("%"));
	}

	m_sessions[user]->part(QString::fromStdString(r));
	m_sessions[user]->removeAutoJoinChannel(QString::fromStdString(r));
}
