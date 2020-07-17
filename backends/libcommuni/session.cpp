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

#include "session.h"
#include <QtCore>
#include <iostream>
#include <IrcCommand>
#include <IrcMessage>
#include <IrcUser>
#include <IrcChannel>
#include "backports.h"

#include "ircnetworkplugin.h"

#define FROM_UTF8(WHAT) QString::fromUtf8((WHAT).c_str(), (WHAT).size())
#define TO_UTF8(WHAT) std::string((WHAT).toUtf8().data(), (WHAT).toUtf8().size())
#define foreach BOOST_FOREACH

#include "transport/Logging.h"

DEFINE_LOGGER(connectionLogger, "IRCConnection");

MyIrcSession::MyIrcSession(const std::string &user, IRCNetworkPlugin *np, const std::string &suffix, QObject* parent) : IrcConnection(parent)
{
	m_np = np;
	m_user = user;
	m_suffix = suffix;
	m_connected = false;
	rooms = 0;
	m_bufferModel = NULL;

	connect(this, SIGNAL(disconnected()), SLOT(on_disconnected()));
	connect(this, SIGNAL(socketError(QAbstractSocket::SocketError)), SLOT(on_socketError(QAbstractSocket::SocketError)));
	connect(this, SIGNAL(connected()), SLOT(on_connected()));

	m_awayTimer = new QTimer(this);
	connect(m_awayTimer, SIGNAL(timeout()), this, SLOT(awayTimeout()));
	m_awayTimer->start(1 * 1000);
}

MyIrcSession::~MyIrcSession() {
	delete m_awayTimer;
}

void MyIrcSession::createBufferModel() {
	m_bufferModel = new IrcBufferModel(this);
	connect(m_bufferModel, SIGNAL(added(IrcBuffer*)), this, SLOT(onBufferAdded(IrcBuffer*)));
	connect(m_bufferModel, SIGNAL(removed(IrcBuffer*)), this, SLOT(onBufferRemoved(IrcBuffer*)));

	// keep the command parser aware of the context
// 	connect(m_bufferModel, SIGNAL(channelsChanged(QStringList)), parser, SLOT(setChannels(QStringList)));

	// create a server buffer for non-targeted messages...
	IrcBuffer* serverBuffer = m_bufferModel->add(host());
	// ...and connect it to IrcBufferModel::messageIgnored()
	// TODO: Make this configurable, so users can show the MOTD and other stuff as in normal
	// IRC client.
	connect(m_bufferModel, SIGNAL(messageIgnored(IrcMessage*)), serverBuffer, SLOT(receiveMessage(IrcMessage*)));
}

void MyIrcSession::onBufferAdded(IrcBuffer* buffer) {
	LOG4CXX_INFO(connectionLogger, m_user << ": Created IrcBuffer " << TO_UTF8(buffer->name()));

	connect(buffer, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));

	if (buffer->isChannel()) {
		QVariantMap userData;
		userData["awayCycle"] = boost::lexical_cast<int>(CONFIG_STRING_DEFAULTED(m_np->getConfig(), "service.irc_away_timeout", "60")) + m_userModels.size();
		userData["awayTick"] = 0;
		buffer->setUserData(userData);
	}

	// create a  model for buffer users
	IrcUserModel* userModel = new IrcUserModel(buffer);
	connect(userModel, SIGNAL(added(IrcUser*)), this, SLOT(onIrcUserAdded(IrcUser*)));
	connect(userModel, SIGNAL(removed(IrcUser*)), this, SLOT(onIrcUserRemoved(IrcUser*)));
	m_userModels.insert(buffer, userModel);
}

void MyIrcSession::onBufferRemoved(IrcBuffer* buffer) {
	LOG4CXX_INFO(connectionLogger, m_user << ": Removed IrcBuffer " << TO_UTF8(buffer->name()));
	// the buffer specific models and documents are no longer needed
	delete m_userModels.take(buffer);
}

bool MyIrcSession::hasIrcUser(const std::string &channel_, const std::string &name) {
	std::string channel = channel_;
	if (channel[0] != '#') {
		channel = "#" + channel;
	}

	IrcBuffer *buffer = m_bufferModel->find(FROM_UTF8(channel));
	if (!buffer) {
		LOG4CXX_ERROR(connectionLogger, m_user << ": Cannot find IrcBuffer '" << channel << "'");
		return false;
	}

	IrcUserModel *userModel = m_userModels.value(buffer);
	if (!userModel) {
		LOG4CXX_ERROR(connectionLogger, m_user << ": Cannot find UserModel for IrcBuffer " << channel);
		return false;
	}

	return userModel->contains(FROM_UTF8(name));
}

void MyIrcSession::sendUserToFrontend(IrcUser *user, pbnetwork::StatusType statusType, const std::string &statusMessage, const std::string &newNick) {
	std::string target = "#" + TO_UTF8(user->channel()->name().toLower()) + m_suffix;
	int op = user->mode() == "o";

	if (statusType != pbnetwork::STATUS_NONE) {
		if (user->isAway()) {
			statusType = pbnetwork::STATUS_AWAY;
		}
		if (newNick.empty()) {
			LOG4CXX_INFO(connectionLogger, m_user << ": IrcUser connected: " << target << "/" << TO_UTF8(user->name()));
		}
		else {
			LOG4CXX_INFO(connectionLogger, m_user << ": IrcUser changed nickname: " << target << "/" << TO_UTF8(user->name()) << ", newNick=" << newNick);
		}
	}
	else {
		LOG4CXX_INFO(connectionLogger, m_user << ": IrcUser disconnected: " << target << "/" << TO_UTF8(user->name()));
	}

	m_np->handleParticipantChanged(m_user, TO_UTF8(user->name()), target, op, statusType, statusMessage, newNick);
}

void MyIrcSession::onIrcUserChanged(bool dummy) {
	IrcUser *user = dynamic_cast<IrcUser *>(QObject::sender());
	if (!user) {
		return;
	}

	LOG4CXX_INFO(connectionLogger, m_user << ": IrcUser " << TO_UTF8(user->name()) << " changed.");
	sendUserToFrontend(user, pbnetwork::STATUS_ONLINE);
}

void MyIrcSession::onIrcUserChanged(const QString &) {
	onIrcUserChanged(false);
}

void MyIrcSession::onIrcUserAdded(IrcUser *user) {
	sendUserToFrontend(user, pbnetwork::STATUS_ONLINE);
	connect(user, SIGNAL(modeChanged(const QString&)), this, SLOT(onIrcUserChanged(const QString&)));
	connect(user, SIGNAL(awayChanged(bool)), this, SLOT(onIrcUserChanged(bool)));
	
}

void MyIrcSession::onIrcUserRemoved(IrcUser *user) {
	sendUserToFrontend(user, pbnetwork::STATUS_NONE);
	disconnect(user, SIGNAL(modeChanged(const QString&)), this, SLOT(onIrcUserChanged(const QString&)));
	disconnect(user, SIGNAL(awayChanged(bool)), this, SLOT(onIrcUserChanged(bool)));
}

void MyIrcSession::on_connected() {
	m_connected = true;
	if (m_suffix.empty()) {
		m_np->handleConnected(m_user);
	}

	if (getIdentify().find(" ") != std::string::npos) {
		std::string to = getIdentify().substr(0, getIdentify().find(" "));
		std::string what = getIdentify().substr(getIdentify().find(" ") + 1);
		LOG4CXX_INFO(connectionLogger, m_user << ": Sending IDENTIFY message to " << to);
		sendCommand(IrcCommand::createMessage(FROM_UTF8(to), FROM_UTF8(what)));
	}
}

void MyIrcSession::addPM(const std::string &name, const std::string &room) {
	LOG4CXX_INFO(connectionLogger, m_user << ": Adding PM conversation " << name << " " << room);
	m_pms[name] = room;
}

void MyIrcSession::on_socketError(QAbstractSocket::SocketError error) {
	std::string reason;
	switch(error) {
		case QAbstractSocket::ConnectionRefusedError: reason = "The connection was refused by the peer (or timed out)."; break;
		case QAbstractSocket::RemoteHostClosedError: reason = "The remote host closed the connection."; break;
		case QAbstractSocket::HostNotFoundError: reason = "The host address was not found."; break;
		case QAbstractSocket::SocketAccessError: reason = "The socket operation failed because the application lacked the required privileges."; break;
		case QAbstractSocket::SocketResourceError: reason = "The local system ran out of resources."; break;
		case QAbstractSocket::SocketTimeoutError: reason = "The socket operation timed out."; break;
		case QAbstractSocket::DatagramTooLargeError: reason = "The datagram was larger than the operating system's limit."; break;
		case QAbstractSocket::NetworkError: reason = "An error occurred with the network."; break;
		case QAbstractSocket::SslHandshakeFailedError: reason = "The SSL/TLS handshake failed, so the connection was closed"; break;
		case QAbstractSocket::UnknownSocketError: reason = "An unidentified error occurred."; break;
		default: reason= "Unknown error."; break;
	};

	if (!m_suffix.empty()) {
		foreach (IrcBuffer *buffer, m_bufferModel->buffers()) {
			if (!buffer->isChannel()) {
				continue;
			}
			m_np->handleParticipantChanged(m_user, TO_UTF8(nickName()), TO_UTF8(buffer->title()) + m_suffix, pbnetwork::PARTICIPANT_FLAG_ROOM_NOT_FOUND, pbnetwork::STATUS_NONE, reason);
		}
	}
	else {
		m_np->handleDisconnected(m_user, 0, reason);
		m_np->tryNextServer();
	}
	LOG4CXX_INFO(connectionLogger, m_user << ": Disconnected from IRC network: " << reason);
	m_connected = false;
}

void MyIrcSession::on_disconnected() {
	if (m_suffix.empty()) {
		m_np->handleDisconnected(m_user, 0, "");
		m_np->tryNextServer();
	}
	m_connected = false;
}

void MyIrcSession::correctNickname(std::string &nick) {
	if (nick.empty()) {
		return;
	}

	switch(nick.at(0)) {
		case '@':
		case '+':
		case '~':
		case '&':
		case '%':
			nick.erase(0, 1);
			break;
		default: break;
	}
}

IrcUser *MyIrcSession::getIrcUser(IrcBuffer *buffer, std::string &nick) {
	correctNickname(nick);
	IrcUserModel *userModel = m_userModels.value(buffer);
	if (!userModel) {
		LOG4CXX_ERROR(connectionLogger, m_user << ": Cannot find UserModel for IrcBuffer " << TO_UTF8(buffer->name()));
		return NULL;
	}

	return userModel->find(FROM_UTF8(nick));
}

IrcUser *MyIrcSession::getIrcUser(IrcBuffer *buffer, IrcMessage *message) {
	std::string nick = TO_UTF8(message->nick());
	return getIrcUser(buffer, nick);
}

void MyIrcSession::on_parted(IrcMessage *message) {
	// TODO: We currently use onIrcUserRemoved, but this does not allow sending
	// part/quit message. We should use this method instead and write version
	// of sendUserToFrontend which takes nickname instead of IrcUser just for
	// part/quit messages.
// 	IrcPartMessage *m = (IrcPartMessage *) message;
// 	IrcBuffer *buffer = dynamic_cast<IrcBuffer *>(QObject::sender());
// 	IrcUser *user = getIrcUser(buffer, message);
// 	if (!user) {
// 		LOG4CXX_ERROR(connectionLogger, m_user << ": Part: IrcUser " << TO_UTF8(message->nick()) << " not in channel " << TO_UTF8(buffer->name()));
// 		return;
// 	}
// 
// 	sendUserToFrontend(user, pbnetwork::STATUS_NONE, TO_UTF8(m->reason()));
}

void MyIrcSession::on_quit(IrcMessage *message) {
	// TODO: We currently use onIrcUserRemoved, but this does not allow sending
	// part/quit message. We should use this method instead and write version
	// of sendUserToFrontend which takes nickname instead of IrcUser just for
	// part/quit messages.
// 	IrcQuitMessage *m = (IrcQuitMessage *) message;
// 	IrcBuffer *buffer = dynamic_cast<IrcBuffer *>(QObject::sender());
// 	IrcUser *user = getIrcUser(buffer, message);
// 	if (!user) {
// 		LOG4CXX_ERROR(connectionLogger, m_user << ": Quit: IrcUser " << TO_UTF8(message->nick()) << " not in channel " << TO_UTF8(buffer->name()));
// 		return;
// 	}
// 
// 	sendUserToFrontend(user, pbnetwork::STATUS_NONE, TO_UTF8(m->reason()));
}

void MyIrcSession::on_nickChanged(IrcMessage *message) {
	IrcNickMessage *m = (IrcNickMessage *) message;
	IrcBuffer *buffer = dynamic_cast<IrcBuffer *>(QObject::sender());
	IrcUser *user = getIrcUser(buffer, message);
	if (!user) {
		LOG4CXX_ERROR(connectionLogger, m_user << ": NickChanged: IrcUser " << TO_UTF8(message->nick()) << " not in channel " << TO_UTF8(buffer->name()));
		return;
	}

	sendUserToFrontend(user, pbnetwork::STATUS_ONLINE, "", TO_UTF8(m->newNick()));
}

void MyIrcSession::on_topicChanged(IrcMessage *message) {
	IrcTopicMessage *m = (IrcTopicMessage *) message;

	std::string nickname = TO_UTF8(m->nick());
	correctNickname(nickname);

	LOG4CXX_INFO(connectionLogger, m_user << ": " << nickname << " topic changed to " << TO_UTF8(m->topic()));
	m_np->handleSubject(m_user, TO_UTF8(m->channel().toLower()) + m_suffix, TO_UTF8(m->topic()), nickname);
}

void MyIrcSession::sendWhoisCommand(const std::string &channel, const std::string &to) {
	m_whois[to] = channel;
	sendCommand(IrcCommand::createWhois(FROM_UTF8(to)));
}

void MyIrcSession::on_whoisMessageReceived(IrcMessage *message) {
	IrcWhoisMessage *m = (IrcWhoisMessage *) message;
	std::string nickname = TO_UTF8(m->nick());
	if (m_whois.find(nickname) == m_whois.end()) {
		LOG4CXX_INFO(connectionLogger, "Whois response received with unexpected nickname " << nickname);
		return;
	}

	std::string msg = "";
	msg += nickname + " is connected to " + TO_UTF8(m->server()) + " (" + TO_UTF8(m->realName()) + ")\n";
	msg += nickname + " is a user on channels: " + TO_UTF8(m->channels().join(", "));

	sendMessageToFrontend(m_whois[nickname], "whois", msg);
	m_whois.erase(nickname);
}

void MyIrcSession::on_namesMessageReceived(IrcMessage *message) {
	LOG4CXX_INFO(connectionLogger, m_user << ": NAMES received");
	IrcBuffer *buffer = dynamic_cast<IrcBuffer *>(QObject::sender());
	IrcUserModel *userModel = m_userModels.value(buffer);
	if (!userModel) {
		LOG4CXX_ERROR(connectionLogger, m_user << ": Cannot find UserModel for IrcBuffer " << TO_UTF8(buffer->name()));
		return;
	}

	foreach (IrcUser *user, userModel->users()) { 
		sendUserToFrontend(user, pbnetwork::STATUS_ONLINE);
	}

	LOG4CXX_INFO(connectionLogger, m_user << "Asking /who for channel " << TO_UTF8(buffer->name()));
	sendCommand(IrcCommand::createWho(buffer->name()));
}

void MyIrcSession::sendMessageToFrontend(const std::string &channel, const std::string &nick, const std::string &msg) {
	QString html = "";//msg;
// 	CommuniBackport::toPlainText(msg);

	// TODO: Communi produces invalid html now...
// 	if (html == msg) {
// 		html = "";
// 	}
// 	else {
// 		html = IrcUtil::messageToHtml(html);
// 	}

	std::string nickname = nick;
	if (channel.find("#") == 0) {
		correctNickname(nickname);
		m_np->handleMessage(m_user, channel + m_suffix, msg, nickname, TO_UTF8(html));
	}
	else {
		correctNickname(nickname);
		if (m_pms.find(nickname) != m_pms.end()) {
			std::string room = m_pms[nickname].substr(0, m_pms[nickname].find("/"));
			room = room.substr(0, room.find("@"));
			if (hasIrcUser(room, nickname)) {
				m_np->handleMessage(m_user, room + m_suffix, msg, nickname, TO_UTF8(html), "", false, true);
				return;
			}
			else {
				nickname = nickname + m_suffix;
			}
		}
		else {
			foreach (IrcBuffer *buffer, m_bufferModel->buffers()) {
				std::string room = "#" + TO_UTF8(buffer->name());
				IrcUserModel *userModel = m_userModels.value(buffer);
				if (!userModel) {
					LOG4CXX_ERROR(connectionLogger, m_user << ": Cannot find UserModel for IrcBuffer " << TO_UTF8(buffer->name()));
					continue;
				}

				if (!userModel->contains(FROM_UTF8(nickname))) {
					continue;
				}

				addPM(nickname, room + "/" + nickname);
				m_np->handleMessage(m_user, room + m_suffix, msg, nickname, TO_UTF8(html), "", false, true);
				return;
			}

			nickname = nickname + m_suffix;
		}

		m_np->handleMessage(m_user, nickname, msg, "", TO_UTF8(html));
	}
}

void MyIrcSession::on_messageReceived(IrcMessage *message) {
	IrcPrivateMessage *m = (IrcPrivateMessage *) message;
	if (m->isRequest()) {
		QString request = m->content().split(" ", QString::SkipEmptyParts).value(0).toUpper();
		if (request == "PING" || request == "TIME" || request == "VERSION") {
			LOG4CXX_INFO(connectionLogger, m_user << ": " << TO_UTF8(request) << " received and has been answered");
			return;
		}
	}

	std::string msg = TO_UTF8(m->content());
	if (m->isAction()) {
		msg = "/me " + msg;
	}

	std::string target = TO_UTF8(m->target().toLower());
	std::string nickname = TO_UTF8(m->nick());
	sendMessageToFrontend(target, nickname, msg);
}

void MyIrcSession::on_numericMessageReceived(IrcMessage *message) {
	QString channel;
	QStringList members;
	std::string nick;

	IrcNumericMessage *m = (IrcNumericMessage *) message;
	QStringList parameters = m->parameters();
	switch (m->code()) {
		case Irc::RPL_TOPIC:
			m_topicData = TO_UTF8(parameters[2]);
			break;
		case Irc::RPL_TOPICWHOTIME:
			nick = TO_UTF8(parameters[2]);
			if (nick.find("!") != std::string::npos) {
				nick = nick.substr(0, nick.find("!"));
			}
			if (nick.find("/") != std::string::npos) {
				nick = nick.substr(0, nick.find("/"));
			}
			m_np->handleSubject(m_user, TO_UTF8(parameters[1].toLower()) + m_suffix, m_topicData, nick);
			break;
		case Irc::ERR_NOSUCHNICK:
		case Irc::ERR_NOSUCHSERVER:
			nick = TO_UTF8(parameters[1]);
			if (m_whois.find(nick) != m_whois.end()) {
				sendMessageToFrontend(m_whois[nick], "whois", nick + ": No such client");
				m_whois.erase(nick);
			}
			break;
		case Irc::ERR_ERRONEUSNICKNAME:
			m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_INVALID_USERNAME, "Erroneous Nickname");
			break;
		case Irc::ERR_NICKNAMEINUSE:
		case Irc::ERR_NICKCOLLISION:
			foreach (IrcBuffer *buffer, m_bufferModel->buffers()) {
				if (!buffer->isChannel()) {
					continue;
				}
				m_np->handleRoomNicknameChanged(m_user, TO_UTF8(buffer->title()) + m_suffix, TO_UTF8(nickName() + "_"));
				m_np->handleParticipantChanged(m_user, TO_UTF8(nickName()), TO_UTF8(buffer->title()) + m_suffix, 0, pbnetwork::STATUS_ONLINE, "", TO_UTF8(nickName() + "_"));
			}
			setNickName(nickName() + "_");
			open();
// 			for(AutoJoinMap::iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
// 				m_np->handleParticipantChanged(m_user, TO_UTF8(nickName()), it->second->getChannel() + m_suffix, pbnetwork::PARTICIPANT_FLAG_CONFLICT);
// 			}
// 			if (m_suffix.empty()) {
// 				m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_INVALID_USERNAME, "Nickname is already in use");
// 			}
			break;
// 			foreach (IrcBuffer *buffer, m_bufferModel->buffers()) {
// 				if (!buffer->isChannel()) {
// 					continue;
// 				}
// 				m_np->handleParticipantChanged(m_user, TO_UTF8(nickName()), TO_UTF8(buffer->title()) + m_suffix, pbnetwork::PARTICIPANT_FLAG_CONFLICT);
// 			}
// 			m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_INVALID_USERNAME, "Nickname collision KILL");
// 			break;
		case Irc::ERR_PASSWDMISMATCH:
			foreach (IrcBuffer *buffer, m_bufferModel->buffers()) {
				if (!buffer->isChannel()) {
					continue;
				}
				m_np->handleParticipantChanged(m_user, TO_UTF8(nickName()), TO_UTF8(buffer->title()) + m_suffix, pbnetwork::PARTICIPANT_FLAG_NOT_AUTHORIZED);
			}
			if (m_suffix.empty()) {
				m_np->handleDisconnected(m_user, pbnetwork::CONNECTION_ERROR_INVALID_USERNAME, "Password incorrect");
			}
		case 321:
			m_rooms.clear();
			m_names.clear();
			break;
		case 322:
			m_rooms.push_back(TO_UTF8(parameters[1]));
			m_names.push_back(TO_UTF8(parameters[1]));
			break;
		case 323:
			m_np->handleRoomList("", m_rooms, m_names);
			break;
		default:
			break;
	}

	if (m->code() >= 400 && m->code() < 500) {
		LOG4CXX_INFO(connectionLogger, m_user << ": Error message received: " << message->toData().data());
	}
	else {
		LOG4CXX_INFO(connectionLogger, m_user << ": Numeric message received: " << message->toData().data());
	}
}

void MyIrcSession::awayTimeout() {
	foreach (IrcBuffer *buffer, m_bufferModel->buffers()) {
		if (!buffer->isChannel()) {
			continue;
		}

		QVariantMap userData = buffer->userData();
		int awayCycle = userData["awayCycle"].toInt();
		int awayTick = userData["awayTick"].toInt();

		if (awayTick == awayCycle) {
			LOG4CXX_INFO(connectionLogger, m_user << ": The time has come. Asking /who " << TO_UTF8(buffer->title()) << " again to get current away states.");
			sendCommand(IrcCommand::createWho(buffer->title()));
			awayTick = 0;
		}
		awayTick++;

		userData["awayCycle"] = awayCycle;
		userData["awayTick"] = awayTick;
		buffer->setUserData(userData);
	}
}

void MyIrcSession::on_noticeMessageReceived(IrcMessage *message) {
	IrcNoticeMessage *m = (IrcNoticeMessage *) message;
	LOG4CXX_INFO(connectionLogger, m_user << ": NOTICE " << TO_UTF8(m->content()));

	std::string msg = TO_UTF8(m->content());
	std::string target = TO_UTF8(m->target().toLower());
	std::string nickname = TO_UTF8(m->nick());
	sendMessageToFrontend(target, nickname, msg);
}

void MyIrcSession::onMessageReceived(IrcMessage *message) {
	switch (message->type()) {
		case IrcMessage::Part:
			on_parted(message);
			break;
		case IrcMessage::Quit:
			on_quit(message);
			break;
		case IrcMessage::Nick:
			on_nickChanged(message);
			break;
		case IrcMessage::Topic:
			on_topicChanged(message);
			break;
		case IrcMessage::Private:
			on_messageReceived(message);
			break;
		case IrcMessage::Numeric:
			on_numericMessageReceived(message);
			break;
		case IrcMessage::Notice:
			on_noticeMessageReceived(message);
			break;
		case IrcMessage::Whois:
			on_whoisMessageReceived(message);
			break;
		case IrcMessage::Names:
			on_namesMessageReceived(message);
			break;
		default:break;
	}
}
