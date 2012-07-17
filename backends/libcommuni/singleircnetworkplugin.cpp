#include "singleircnetworkplugin.h"
#include "transport/logging.h"
#include <IrcCommand>
#include <IrcMessage>

#define FROM_UTF8(WHAT) QString::fromUtf8((WHAT).c_str(), (WHAT).size())
#define TO_UTF8(WHAT) std::string((WHAT).toUtf8().data(), (WHAT).toUtf8().size())

DEFINE_LOGGER(logger, "SingleIRCNetworkPlugin");

SingleIRCNetworkPlugin::SingleIRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port) {
	this->config = config;
	if (CONFIG_HAS_KEY(config, "service.irc_server")) {
		m_server = CONFIG_STRING(config, "service.irc_server");
	}
	else {
		LOG4CXX_ERROR(logger, "No [service] irc_server defined, exiting...");
		exit(-1);
	}
	m_socket = new QTcpSocket();
	m_socket->connectToHost(FROM_UTF8(host), port);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));

	if (CONFIG_HAS_KEY(config, "service.irc_identify")) {
		m_identify = CONFIG_STRING(config, "service.irc_identify");
	}
	else {
		m_identify = "NickServ identify $name $password";
	}

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
	session->setUserName(FROM_UTF8(legacyName));
	session->setNickName(FROM_UTF8(legacyName));
	session->setRealName(FROM_UTF8(legacyName));
	session->setHost(FROM_UTF8(m_server));
	session->setPort(6667);
	session->setEncoding( "utf-8" );

	if (!password.empty()) {
		std::string identify = m_identify;
		boost::replace_all(identify, "$password", password);
		boost::replace_all(identify, "$name", legacyName);
		session->setIdentify(identify);
	}

	session->open();

	m_sessions[user] = session;
}

void SingleIRCNetworkPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) {
	if (m_sessions[user] == NULL) {
		LOG4CXX_WARN(logger, user << ": Already disconnected.");
		return;
	}
	LOG4CXX_INFO(logger, user << ": Disconnecting.");

	m_sessions[user]->close();
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
	m_sessions[user]->sendCommand(IrcCommand::createMessage(FROM_UTF8(r), FROM_UTF8(message)));

	if (r.find("#") == 0) {
		handleMessage(user, legacyName, message, TO_UTF8(m_sessions[user]->nickName()));
	}
}

void SingleIRCNetworkPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
	if (m_sessions[user] == NULL) {
		LOG4CXX_WARN(logger, user << ": Join room requested for unconnected user");
		return;
	}

	LOG4CXX_INFO(logger, user << ": Joining " << room);
	m_sessions[user]->addAutoJoinChannel(room);
	m_sessions[user]->sendCommand(IrcCommand::createJoin(FROM_UTF8(room), FROM_UTF8(password)));
	m_sessions[user]->rooms += 1;

	// update nickname, because we have nickname per session, no nickname per room.
	handleRoomNicknameChanged(user, room, TO_UTF8(m_sessions[user]->userName()));
}

void SingleIRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::string r = room;
	std::string u = user;

	if (m_sessions[u] == NULL) {
		LOG4CXX_WARN(logger, user << ": Leave room requested for unconnected user");
		return;
	}

	LOG4CXX_INFO(logger, user << ": Leaving " << room);
	m_sessions[u]->sendCommand(IrcCommand::createPart(FROM_UTF8(r)));
	m_sessions[u]->removeAutoJoinChannel(r);
	m_sessions[u]->rooms -= 1;
}
