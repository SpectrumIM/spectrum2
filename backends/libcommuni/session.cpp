/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "session.h"
#include <QtCore>
#include <iostream>
#include <IrcCommand>
#include <IrcMessage>

#include "ircnetworkplugin.h"

#define FROM_UTF8(WHAT) QString::fromUtf8((WHAT).c_str(), (WHAT).size())
#define TO_UTF8(WHAT) std::string((WHAT).toUtf8().data(), (WHAT).toUtf8().size())

#include "transport/logging.h"

DEFINE_LOGGER(logger, "IRCSession");

MyIrcSession::MyIrcSession(const std::string &user, IRCNetworkPlugin *np, const std::string &suffix, QObject* parent) : IrcSession(parent)
{
	this->np = np;
	this->user = user;
	this->suffix = suffix;
	m_connected = false;
	rooms = 0;
	connect(this, SIGNAL(disconnected()), SLOT(on_disconnected()));
	connect(this, SIGNAL(socketError(QAbstractSocket::SocketError)), SLOT(on_socketError(QAbstractSocket::SocketError)));
	connect(this, SIGNAL(connected()), SLOT(on_connected()));
	connect(this, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
}

void MyIrcSession::on_connected() {
	m_connected = true;
	if (suffix.empty()) {
		np->handleConnected(user);
	}

	for(AutoJoinMap::iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		sendCommand(IrcCommand::createJoin(FROM_UTF8(it->second->getChannel()), FROM_UTF8(it->second->getPassword())));
	}

	if (getIdentify().find(" ") != std::string::npos) {
		std::string to = getIdentify().substr(0, getIdentify().find(" "));
		std::string what = getIdentify().substr(getIdentify().find(" ") + 1);
		sendCommand(IrcCommand::createMessage(FROM_UTF8(to), FROM_UTF8(what)));
	}
}

void MyIrcSession::on_socketError(QAbstractSocket::SocketError error) {
	on_disconnected();
}

void MyIrcSession::on_disconnected() {
	if (suffix.empty()) {
		np->handleDisconnected(user, 0, "");
		np->tryNextServer();
	}
	m_connected = false;
}

bool MyIrcSession::correctNickname(std::string &nickname) {
	bool flags = 0;
	switch(nickname.at(0)) {
		case '@': nickname = nickname.substr(1); flags = 1; break;
		case '+': nickname = nickname.substr(1); break;
		default: break;
	}
	return flags;
}

void MyIrcSession::on_joined(IrcMessage *message) {
	IrcJoinMessage *m = (IrcJoinMessage *) message;
	bool flags = 0;
	std::string nickname = TO_UTF8(m->sender().name());
	flags = correctNickname(nickname);
	np->handleParticipantChanged(user, nickname, TO_UTF8(m->channel()), (int) flags, pbnetwork::STATUS_ONLINE);
	LOG4CXX_INFO(logger, user << ": Joined " << TO_UTF8(m->parameters()[0]));
}


void MyIrcSession::on_parted(IrcMessage *message) {
	IrcPartMessage *m = (IrcPartMessage *) message;
	bool flags = 0;
	std::string nickname = TO_UTF8(m->sender().name());
	flags = correctNickname(nickname);
	LOG4CXX_INFO(logger, user << ": " << nickname << " parted " << TO_UTF8(m->channel()) + suffix);
	np->handleParticipantChanged(user, nickname, TO_UTF8(m->channel()) + suffix,(int) flags, pbnetwork::STATUS_NONE, TO_UTF8(m->reason()));
}

void MyIrcSession::on_quit(IrcMessage *message) {
	IrcQuitMessage *m = (IrcQuitMessage *) message;
	for(AutoJoinMap::iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		bool flags = 0;
		std::string nickname = TO_UTF8(m->sender().name());
		flags = correctNickname(nickname);
		LOG4CXX_INFO(logger, user << ": " << nickname << " quit " << it->second->getChannel() + suffix);
		np->handleParticipantChanged(user, nickname, it->second->getChannel() + suffix,(int) flags, pbnetwork::STATUS_NONE, TO_UTF8(m->reason()));
	}
}

void MyIrcSession::on_nickChanged(IrcMessage *message) {
	IrcNickMessage *m = (IrcNickMessage *) message;

	for(AutoJoinMap::iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		std::string nickname = TO_UTF8(m->sender().name());
		bool flags = m_modes[it->second->getChannel() + nickname];
		LOG4CXX_INFO(logger, user << ": " << nickname << " changed nickname to " << TO_UTF8(m->nick()));
		np->handleParticipantChanged(user, nickname, it->second->getChannel() + suffix,(int) flags, pbnetwork::STATUS_ONLINE, "", TO_UTF8(m->nick()));
	}
}

void MyIrcSession::on_modeChanged(IrcMessage *message) {
	IrcModeMessage *m = (IrcModeMessage *) message;

	// mode changed: "#testik" "HanzZ" "+o" "hanzz_k"
	std::string nickname = TO_UTF8(m->argument());
	std::string mode = TO_UTF8(m->mode());
	if (nickname.empty())
		return;
	LOG4CXX_INFO(logger, user << ": " << nickname << " changed mode to " << mode);
	for(AutoJoinMap::iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		if (mode == "+o") {
			m_modes[it->second->getChannel() + nickname] = 1;
		}
		else {
			m_modes[it->second->getChannel() + nickname] = 0;
		}
		bool flags = m_modes[it->second->getChannel() + nickname];
		np->handleParticipantChanged(user, nickname, it->second->getChannel() + suffix,(int) flags, pbnetwork::STATUS_ONLINE, "");
	}
}

void MyIrcSession::on_topicChanged(IrcMessage *message) {
	IrcTopicMessage *m = (IrcTopicMessage *) message;

	bool flags = 0;
	std::string nickname = TO_UTF8(m->sender().name());
	flags = correctNickname(nickname);

	LOG4CXX_INFO(logger, user << ": " << nickname << " topic changed to " << TO_UTF8(m->topic()));
	np->handleSubject(user, TO_UTF8(m->channel()) + suffix, TO_UTF8(m->topic()), nickname);
}

void MyIrcSession::on_messageReceived(IrcMessage *message) {
	IrcPrivateMessage *m = (IrcPrivateMessage *) message;
	if (m->isRequest()) {
		QString request = m->message().split(" ", QString::SkipEmptyParts).value(0).toUpper();
		if (request == "PING" || request == "TIME" || request == "VERSION") {
			LOG4CXX_INFO(logger, user << ": " << TO_UTF8(request) << " received and has been answered");
			return;
		}
	}

	std::string target = TO_UTF8(m->target());
	LOG4CXX_INFO(logger, user << ": Message from " << target);
	if (target.find("#") == 0) {
		bool flags = 0;
		std::string nickname = TO_UTF8(m->sender().name());
		flags = correctNickname(nickname);
		np->handleMessage(user, target + suffix, TO_UTF8(m->message()), nickname);
	}
	else {
		bool flags = 0;
		std::string nickname = TO_UTF8(m->sender().name());
		flags = correctNickname(nickname);
		LOG4CXX_INFO(logger, nickname + suffix);
		np->handleMessage(user, nickname + suffix, TO_UTF8(m->message()));
	}
}

void MyIrcSession::on_numericMessageReceived(IrcMessage *message) {
	QString channel;
	QStringList members;

	IrcNumericMessage *m = (IrcNumericMessage *) message;
	switch (m->code()) {
		case 332:
			m_topicData = TO_UTF8(m->parameters().value(2));
			break;
		case 333:
			 np->handleSubject(user, TO_UTF8(m->parameters().value(1)) + suffix, m_topicData, TO_UTF8(m->parameters().value(2)));
			break;
		case 353:
			channel = m->parameters().value(2);
			members = m->parameters().value(3).split(" ");

			LOG4CXX_INFO(logger, user << ": Received members for " << TO_UTF8(channel) << suffix);
			for (int i = 0; i < members.size(); i++) {
				bool flags = 0;
				std::string nickname = TO_UTF8(members.at(i));
				flags = correctNickname(nickname);
				m_modes[TO_UTF8(channel) + nickname] = flags;
				np->handleParticipantChanged(user, nickname, TO_UTF8(channel) + suffix,(int) flags, pbnetwork::STATUS_ONLINE);
			}
			break;
		case 432:
			np->handleDisconnected(user, pbnetwork::CONNECTION_ERROR_INVALID_USERNAME, "Erroneous Nickname");
			break;
		default:
			break;
	}

	//qDebug() << "numeric message received:" << receiver() << origin << code << params;
}

void MyIrcSession::onMessageReceived(IrcMessage *message) {
	LOG4CXX_INFO(logger, user << ": " << TO_UTF8(message->toString()));
	switch (message->type()) {
		case IrcMessage::Join:
			on_joined(message);
			break;
		case IrcMessage::Part:
			on_parted(message);
			break;
		case IrcMessage::Quit:
			on_quit(message);
			break;
		case IrcMessage::Nick:
			on_nickChanged(message);
			break;
		case IrcMessage::Mode:
			on_modeChanged(message);
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
		default:break;
	}
}
