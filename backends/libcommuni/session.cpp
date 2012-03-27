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
#include "Swiften/Elements/StatusShow.h"
#include <IrcCommand>
#include <IrcMessage>

#include "transport/logging.h"

DEFINE_LOGGER(logger, "IRCSession");

MyIrcSession::MyIrcSession(const std::string &user, NetworkPlugin *np, const std::string &suffix, QObject* parent) : IrcSession(parent)
{
	this->np = np;
	this->user = user;
	this->suffix = suffix;
	m_connected = false;
	rooms = 0;
	connect(this, SIGNAL(disconnected()), SLOT(on_disconnected()));
	connect(this, SIGNAL(connected()), SLOT(on_connected()));
	connect(this, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
}

void MyIrcSession::on_connected() {
	m_connected = true;
	if (suffix.empty()) {
		np->handleConnected(user);
	}

	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		sendCommand(IrcCommand::createJoin(QString::fromUtf8(*it)));
	}

	if (getIdentify().find(" ") != std::string::npos) {
		std::string to = getIdentify().substr(0, getIdentify().find(" "));
		std::string what = getIdentify().substr(getIdentify().find(" ") + 1);
		sendCommand(IrcCommand::createMessage(QString::fromUtf8(to), QString::fromUtf8(what)));
	}
}

void MyIrcSession::on_disconnected() {
	if (suffix.empty())
		np->handleDisconnected(user, 0, "");
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
	std::string nickname = m->sender().name().toUtf8();
	flags = correctNickname(nickname);
	np->handleParticipantChanged(user, nickname, m->channel().toUtf8() + suffix, (int) flags, pbnetwork::STATUS_ONLINE);
	LOG4CXX_INFO(logger, user << ": Joined " << m->parameters()[0].toUtf8());
}


void MyIrcSession::on_parted(IrcMessage *message) {
	IrcPartMessage *m = (IrcPartMessage *) message;
	bool flags = 0;
	std::string nickname = m->sender().name().toUtf8();
	flags = correctNickname(nickname);
	LOG4CXX_INFO(logger, user << ": " << nickname << " parted " << m->channel().toUtf8() + suffix);
	np->handleParticipantChanged(user, nickname, m->channel().toUtf8() + suffix,(int) flags, pbnetwork::STATUS_NONE, m->reason().toUtf8());
}

void MyIrcSession::on_quit(IrcMessage *message) {
	IrcQuitMessage *m = (IrcQuitMessage *) message;
	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		bool flags = 0;
		std::string nickname = m->sender().name().toUtf8();
		flags = correctNickname(nickname);
		LOG4CXX_INFO(logger, user << ": " << nickname << " quit " << (*it) + suffix);
		np->handleParticipantChanged(user, nickname, (*it) + suffix,(int) flags, pbnetwork::STATUS_NONE, m->reason().toUtf8());
	}
}

void MyIrcSession::on_nickChanged(IrcMessage *message) {
	IrcNickMessage *m = (IrcNickMessage *) message;

	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		std::string nickname = m->sender().name().toUtf8();
		bool flags = m_modes[(*it) + nickname];
		LOG4CXX_INFO(logger, user << ": " << nickname << " changed nickname to " << m->nick().toUtf8());
		np->handleParticipantChanged(user, nickname, (*it) + suffix,(int) flags, pbnetwork::STATUS_ONLINE, "", m->nick().toUtf8());
	}
}

void MyIrcSession::on_modeChanged(IrcMessage *message) {
	IrcModeMessage *m = (IrcModeMessage *) message;

	// mode changed: "#testik" "HanzZ" "+o" "hanzz_k"
	std::string nickname = m->argument().toUtf8();
	std::string mode = m->mode().toUtf8();
	if (nickname.empty())
		return;
	LOG4CXX_INFO(logger, user << ": " << nickname << " changed mode to " << mode);
	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		if (mode == "+o") {
			m_modes[(*it) + nickname] = 1;
		}
		else {
			m_modes[(*it) + nickname] = 0;
		}
		bool flags = m_modes[(*it) + nickname];
		np->handleParticipantChanged(user, nickname, (*it) + suffix,(int) flags, pbnetwork::STATUS_ONLINE, "");
	}
}

void MyIrcSession::on_topicChanged(IrcMessage *message) {
	IrcTopicMessage *m = (IrcTopicMessage *) message;

	bool flags = 0;
	std::string nickname = m->sender().name().toUtf8();
	flags = correctNickname(nickname);

	LOG4CXX_INFO(logger, user << ": " << nickname << " topic changed to " << m->topic().toUtf8());
	np->handleSubject(user, m->channel().toUtf8() + suffix, m->topic().toUtf8(), nickname);
}

void MyIrcSession::on_messageReceived(IrcMessage *message) {
	IrcPrivateMessage *m = (IrcPrivateMessage *) message;

	std::string target = m->target().toUtf8();
	LOG4CXX_INFO(logger, user << ": Message from " << target);
	if (target.find("#") == 0) {
		bool flags = 0;
		std::string nickname = m->sender().name().toUtf8();
		flags = correctNickname(nickname);
		np->handleMessage(user, target + suffix, m->message().toUtf8(), nickname);
	}
	else {
		bool flags = 0;
		std::string nickname = m->sender().name().toUtf8();
		flags = correctNickname(nickname);
		np->handleMessage(user, nickname, m->message().toUtf8());
	}
}

void MyIrcSession::on_numericMessageReceived(IrcMessage *message) {
	QString channel;
	QStringList members;

	IrcNumericMessage *m = (IrcNumericMessage *) message;
	switch (m->code()) {
		case 332:
			m_topicData = m->parameters().value(2).toUtf8();
			break;
		case 333:
			 np->handleSubject(user, m->parameters().value(1).toUtf8() + suffix, m_topicData, m->parameters().value(2).toUtf8());
			break;
		case 353:
			channel = m->parameters().value(2);
			members = m->parameters().value(3).split(" ");

			for (int i = 0; i < members.size(); i++) {
				bool flags = 0;
				std::string nickname = members.at(i).toUtf8();
				flags = correctNickname(nickname);
				m_modes[channel.toUtf8() + nickname] = flags;
				np->handleParticipantChanged(user, nickname, channel.toUtf8() + suffix,(int) flags, pbnetwork::STATUS_ONLINE);
			}
			break;
		case 432:
			if (m_connected) {
				np->handleDisconnected(user, pbnetwork::CONNECTION_ERROR_INVALID_USERNAME, "Erroneous Nickname");
			}
			break;
		default:
			break;
	}

	//qDebug() << "numeric message received:" << receiver() << origin << code << params;
}

void MyIrcSession::onMessageReceived(IrcMessage *message) {
	LOG4CXX_INFO(logger, user << ": " << message->toString().toUtf8());
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
