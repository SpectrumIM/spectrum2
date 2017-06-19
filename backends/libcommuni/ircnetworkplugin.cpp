/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2013, Jan Kaluza <hanzz.k@gmail.com>
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
#include <IrcCommand>
#include <IrcMessage>
#include "ircnetworkplugin.h"
#include "transport/Logging.h"

DEFINE_LOGGER(logger, "IRCNetworkPlugin");

#define FROM_UTF8(WHAT) QString::fromUtf8((WHAT).c_str(), (WHAT).size())
#define TO_UTF8(WHAT) std::string((WHAT).toUtf8().data(), (WHAT).toUtf8().size())

IRCNetworkPlugin::IRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port) {
	m_config = config;
	m_currentServer = 0;
	m_firstPing = true;

	m_socket = new QTcpSocket();
	m_socket->connectToHost(FROM_UTF8(host), port);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readData()));

	std::string server = CONFIG_STRING_DEFAULTED(m_config, "service.irc_server", "");
	if (!server.empty()) {
		m_servers.push_back(server);
	}
	else {
		std::list<std::string> list;
		list = CONFIG_LIST_DEFAULTED(m_config, "service.irc_server", list);
		m_servers.insert(m_servers.begin(), list.begin(), list.end());
	}

	if (CONFIG_HAS_KEY(m_config, "service.irc_identify")) {
		m_identify = CONFIG_STRING(m_config, "service.irc_identify");
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

	if (m_firstPing) {
		m_firstPing = false;

		NetworkPlugin::PluginConfig cfg;
		cfg.setNeedRegistration(false);
		cfg.setSupportMUC(true);
		cfg.disableJIDEscaping();
		sendConfig(cfg);
	}

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
// 	session->setEncoding("UTF8");

	std::vector<std::string> hostname_parts;
	boost::split(hostname_parts, hostname, boost::is_any_of(":/"));
	if (hostname_parts.size() == 2 && !hostname_parts[0].empty() && !hostname_parts[1].empty()) { // hostname was splitted
		session->setHost(FROM_UTF8(hostname_parts[0])); // real hostname
		int port = atoi(hostname_parts[1].c_str()); // user port
		if (hostname_parts[1][0] == '+' || port == 6697) { // use SSL
			port = (port < 1 || port > 65535) ? 6697 : port; // default to standard SSL port
			session->setSecure(true);
		} else { // use TCP
			port = (port < 1 || port > 65535) ? 6667 : port; // default to standart TCP port
		}
		session->setPort(port);
	} else { // hostname was not splitted: default to old behaviour
		session->setHost(FROM_UTF8(hostname));
		session->setPort(6667);
	}

	if (!password.empty()) {
		std::string identify = m_identify;
		boost::replace_all(identify, "$password", password);
		boost::replace_all(identify, "$name", nickname);
		if (CONFIG_BOOL_DEFAULTED(m_config, "service.irc_send_pass", false)) {
			session->setPassword(FROM_UTF8(password)); // use IRC PASS
		} else {
			session->setIdentify(identify); // use identify supplied
		}
	}

	session->createBufferModel();
	LOG4CXX_INFO(logger, user << ": Connecting " << hostname << " as " << nickname << ", port=" << session->port() << ", suffix=" << suffix);

	return session;
}

void IRCNetworkPlugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
	if (!m_servers.empty()) {
		// legacy name is user's nickname
		if (m_sessions[user] != NULL) {
			LOG4CXX_WARN(logger, user << ": Already logged in.");
			return;
		}

		m_sessions[user] = createSession(user, m_servers[m_currentServer], legacyName, password, "");
		m_sessions[user]->open();
	}
	else {
		// We are waiting for first room join to connect user to IRC network, because we don't know which
		// network he chooses...
		LOG4CXX_INFO(logger, user << ": Ready for connections");
		handleConnected(user);
	}
}

void IRCNetworkPlugin::handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id) {
	handleVCard(user, id, legacyName, "", "", "");
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
	if (!CONFIG_BOOL(m_config, "service.server_mode") && m_servers.empty()) {
		u = user + legacyName.substr(legacyName.find("@") + 1);
		if (u.find("/") != std::string::npos) {
			u = u.substr(0, u.find("/"));
		}
	}
	return u;
}

std::string IRCNetworkPlugin::getTargetName(const std::string &legacyName) {
	std::string r = legacyName;
	if (legacyName.find("/") == std::string::npos) {
		r = legacyName.substr(0, r.find("@"));
	}
	else {
		r = legacyName.substr(legacyName.find("/") + 1);
	}
	return r;
}

void IRCNetworkPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/, const std::string &/*id*/) {
	std::string session = getSessionName(user, legacyName);
	if (m_sessions[session] == NULL) {
		LOG4CXX_WARN(logger, user << ": Session name: " << session << ", No session for user");
		return;
	}

	std::string target = getTargetName(legacyName);
	// We are sending PM message. On XMPP side, user is sending PM using the particular channel,
	// for example #room@irc.freenode.org/hanzz. On IRC side, we are forwarding this message
	// just to "hanzz". Therefore we have to somewhere store, that message from "hanzz" should
	// be mapped to #room@irc.freenode.org/hanzz.
	if (legacyName.find("/") != std::string::npos) {
		m_sessions[session]->addPM(target, legacyName.substr(0, legacyName.find("@")));
	}

	LOG4CXX_INFO(logger, user << ": Session name: " << session << ", message to " << target);

	if (message.find("/me") == 0) {
		m_sessions[session]->sendCommand(IrcCommand::createCtcpAction(FROM_UTF8(target), FROM_UTF8(message.substr(4))));
	}
	else if (message.find("/whois") == 0 || message.find(".whois") == 0) {
		m_sessions[session]->sendWhoisCommand(target, message.substr(7));
		return;
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
	bool createdSession = false;
	if (m_sessions[session] == NULL) {
		if (m_servers.empty()) {
			// in gateway mode we want to login this user to network according to legacyName
			if (room.find("@") != std::string::npos) {
				// suffix is %irc.freenode.net to let MyIrcSession return #room%irc.freenode.net
				m_sessions[session] = createSession(user, room.substr(room.find("@") + 1), nickname, "", room.substr(room.find("@")));
				createdSession = true;
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

	m_sessions[session]->sendCommand(IrcCommand::createJoin(FROM_UTF8(target), FROM_UTF8(password)));
	m_sessions[session]->rooms += 1;

	if (createdSession) {
		m_sessions[session]->open();
	}

	// update nickname, because we have nickname per session, no nickname per room.
	if (nickname != TO_UTF8(m_sessions[session]->nickName())) {
		handleRoomNicknameChanged(user, room, TO_UTF8(m_sessions[session]->nickName()));
		handleParticipantChanged(user, nickname, room, 0, pbnetwork::STATUS_ONLINE, "", TO_UTF8(m_sessions[session]->nickName()));
	}
}

void IRCNetworkPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room) {
	std::string session = getSessionName(user, room);
	std::string target = getTargetName(room);

	LOG4CXX_INFO(logger, user << ": Session name: " << session << ", Leaving room " << target);
	if (m_sessions[session] == NULL)
		return;

	m_sessions[session]->sendCommand(IrcCommand::createPart(FROM_UTF8(target)));
	m_sessions[session]->rooms -= 1;

	if (m_sessions[session]->rooms <= 0 && m_servers.empty()) {
		LOG4CXX_INFO(logger, user << ": Session name: " << session << ", User is not in any room, disconnecting from network");
		m_sessions[session]->close();
		m_sessions[session]->deleteLater();
		m_sessions.erase(session);
	}
}

void IRCNetworkPlugin::handleStatusChangeRequest(const std::string &user, int status, const std::string &statusMessage) {
	if (m_sessions[user] == NULL) {
		return;
	}

	if (status == pbnetwork::STATUS_AWAY) {
		LOG4CXX_INFO(logger, user << ": User is now away.");
		m_sessions[user]->sendCommand(IrcCommand::createAway(statusMessage.empty() ? "Away" : FROM_UTF8(statusMessage)));
	}
	else {
		LOG4CXX_INFO(logger, user << ": User is not away anymore.");
		m_sessions[user]->sendCommand(IrcCommand::createAway(""));
	}
}
