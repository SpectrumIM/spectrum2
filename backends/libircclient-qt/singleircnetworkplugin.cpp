#include "singleircnetworkplugin.h"
#include "log4cxx/logger.h"

using namespace log4cxx;

static LoggerPtr logger = log4cxx::Logger::getLogger("SingleIRCNetworkPlugin");

SingleIRCNetworkPlugin::SingleIRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port) {
	this->config = config;
	m_server = config->getUnregistered().find("service.irc_server")->second;
	m_socket = new QTcpSocket();
	m_socket->connectToHost(QString::fromStdString(host), port);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));

	LOG4CXX_INFO(logger, "SingleIRCNetworkPlugin for server " << m_server << " initialized.");
}

void SingleIRCNetworkPlugin::readData() {
	size_t availableBytes = m_socket->bytesAvailable();
	if (availableBytes == 0)
		return;

	std::string d = std::string(m_socket->readAll().data(), availableBytes);
	handleDataRead(d);
}

void SingleIRCNetworkPlugin::sendData(const std::string &string) {
	m_socket->write(string.c_str(), string.size());
}

void SingleIRCNetworkPlugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
	// legacy name is users nickname
	if (m_sessions[user] != NULL) {
		LOG4CXX_WARN(logger, user << ": Already logged in.");
		return;
	}
	LOG4CXX_INFO(logger, user << ": Connecting " << m_server << " as " << legacyName);

	MyIrcSession *session = new MyIrcSession(user, this);
	session->setNick(QString::fromStdString(legacyName));
	session->connectToServer(QString::fromStdString(m_server), 6667);
	m_sessions[user] = session;
}

void SingleIRCNetworkPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
	if (m_sessions[user] == NULL) {
		LOG4CXX_WARN(logger, user << ": Already disconnected.");
		return;
	}
	LOG4CXX_INFO(logger, user << ": Disconnecting.");

	m_sessions[user]->disconnectFromServer();
	m_sessions[user]->deleteLater();
	m_sessions.erase(user);
}

void SingleIRCNetworkPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/) {
	if (m_sessions[user] == NULL) {
		LOG4CXX_WARN(logger, user << ": Message received for unconnected user");
		return;
	}

	// handle PMs
	std::string r = legacyName;
	if (legacyName.find("/") == std::string::npos) {
		r = legacyName.substr(0, r.find("@"));
	}
	else {
		r = legacyName.substr(legacyName.find("/") + 1);
	}

	LOG4CXX_INFO(logger, user << ": Forwarding message to " << r);
	m_sessions[user]->message(QString::fromStdString(r), QString::fromStdString(message));
}

void SingleIRCNetworkPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
	if (m_sessions[user] == NULL) {
		LOG4CXX_WARN(logger, user << ": Join room requested for unconnected user");
		return;
	}

	LOG4CXX_INFO(logger, user << ": Joining " << room);
	m_sessions[user]->addAutoJoinChannel(QString::fromStdString(room));
	m_sessions[user]->join(QString::fromStdString(room), QString::fromStdString(password));
	m_sessions[user]->rooms += 1;

	// update nickname, because we have nickname per session, no nickname per room.
	handleRoomNicknameChanged(user, room, m_sessions[user]->nick().toStdString());
}

void SingleIRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::string r = room;
	std::string u = user;

	if (m_sessions[u] == NULL) {
		LOG4CXX_WARN(logger, user << ": Leave room requested for unconnected user");
		return;
	}

	LOG4CXX_INFO(logger, user << ": Leaving " << room);
	m_sessions[u]->part(QString::fromStdString(r));
	m_sessions[u]->removeAutoJoinChannel(QString::fromStdString(r));
	m_sessions[u]->rooms -= 1;
}
