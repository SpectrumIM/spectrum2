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
	MyIrcSession *session = new MyIrcSession(user, this);
	std::string h = user.substr(0, user.find("@"));
	session->setNick(QString::fromStdString(h.substr(0, h.find("%"))));
	session->connectToServer(QString::fromStdString(h.substr(h.find("%") + 1)), 6667);
	std::cout << "CONNECTING IRC NETWORK " << h.substr(h.find("%") + 1) << "\n";
	m_sessions[user] = session;
}

void IRCNetworkPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
	if (m_sessions[user] == NULL)
		return;
	m_sessions[user]->disconnectFromServer();
	m_sessions[user]->deleteLater();
}

void IRCNetworkPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/) {
	std::cout << "MESSAGE " << user << " " << legacyName << "\n";
	if (m_sessions[user] == NULL)
		return;
	m_sessions[user]->message(QString::fromStdString(legacyName), QString::fromStdString(message));
	std::cout << "SENT\n";
}

void IRCNetworkPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
	std::cout << "JOIN\n";
	if (m_sessions[user] == NULL)
		return;
	m_sessions[user]->addAutoJoinChannel(QString::fromStdString(room));
	m_sessions[user]->join(QString::fromStdString(room), QString::fromStdString(password));
	// update nickname, because we have nickname per session, no nickname per room.
	handleRoomNicknameChanged(user, room, m_sessions[user]->nick().toStdString());
}

void IRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::cout << "PART\n";
	if (m_sessions[user] == NULL)
		return;
	m_sessions[user]->part(QString::fromStdString(room));
	m_sessions[user]->removeAutoJoinChannel(QString::fromStdString(room));
}
