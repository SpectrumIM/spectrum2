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

#include "log4cxx/logger.h"

using namespace log4cxx;

static LoggerPtr logger = log4cxx::Logger::getLogger("IRCSession");

MyIrcSession::MyIrcSession(const std::string &user, NetworkPlugin *np, const std::string &suffix, QObject* parent) : IrcSession(parent)
{
	this->np = np;
	this->user = user;
	this->suffix = suffix;
	rooms = 0;
	connect(this, SIGNAL(disconnected()), SLOT(on_disconnected()));
	connect(this, SIGNAL(connected()), SLOT(on_connected()));
	connect(this, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
}

void MyIrcSession::on_connected() {
	if (suffix.empty()) {
		np->handleConnected(user);
	}

	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		sendCommand(IrcCommand::createJoin(QString::fromStdString(*it)));
	}

	if (getIdentify().find(" ") != std::string::npos) {
		std::string to = getIdentify().substr(0, getIdentify().find(" "));
		std::string what = getIdentify().substr(getIdentify().find(" ") + 1);
		sendCommand(IrcCommand::createMessage(QString::fromStdString(to), QString::fromStdString(what)));
	}
}

void MyIrcSession::on_disconnected() {
	if (suffix.empty())
		np->handleDisconnected(user, 0, "");
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
	std::string nickname = m->sender().name().toStdString();
	flags = correctNickname(nickname);
	np->handleParticipantChanged(user, nickname, m->channel().toStdString() + suffix, (int) flags, pbnetwork::STATUS_ONLINE);
	LOG4CXX_INFO(logger, user << ": Joined " << m->parameters()[0].toStdString());
}


void MyIrcSession::on_parted(IrcMessage *message) {
	IrcPartMessage *m = (IrcPartMessage *) message;
	bool flags = 0;
	std::string nickname = m->sender().name().toStdString();
	flags = correctNickname(nickname);
	LOG4CXX_INFO(logger, user << ": " << nickname << " parted " << m->channel().toStdString() + suffix);
	np->handleParticipantChanged(user, nickname, m->channel().toStdString() + suffix,(int) flags, pbnetwork::STATUS_NONE, m->reason().toStdString());
}

void MyIrcSession::on_quit(IrcMessage *message) {
	IrcQuitMessage *m = (IrcQuitMessage *) message;
	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		bool flags = 0;
		std::string nickname = m->sender().name().toStdString();
		flags = correctNickname(nickname);
		LOG4CXX_INFO(logger, user << ": " << nickname << " quit " << (*it) + suffix);
		np->handleParticipantChanged(user, nickname, (*it) + suffix,(int) flags, pbnetwork::STATUS_NONE, m->reason().toStdString());
	}
}

void MyIrcSession::on_nickChanged(IrcMessage *message) {
	IrcNickMessage *m = (IrcNickMessage *) message;

	for(std::list<std::string>::const_iterator it = m_autoJoin.begin(); it != m_autoJoin.end(); it++) {
		std::string nickname = m->sender().name().toStdString();
		bool flags = m_modes[(*it) + nickname];
		LOG4CXX_INFO(logger, user << ": " << nickname << " changed nickname to " << m->nick().toStdString());
		np->handleParticipantChanged(user, nickname, (*it) + suffix,(int) flags, pbnetwork::STATUS_ONLINE, "", m->nick().toStdString());
	}
}

void MyIrcSession::on_modeChanged(IrcMessage *message) {
	IrcModeMessage *m = (IrcModeMessage *) message;

	// mode changed: "#testik" "HanzZ" "+o" "hanzz_k"
	std::string nickname = m->argument().toStdString();
	std::string mode = m->mode().toStdString();
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
	std::string nickname = m->sender().name().toStdString();
	flags = correctNickname(nickname);

	LOG4CXX_INFO(logger, user << ": " << nickname << " topic changed to " << m->topic().toStdString());
	np->handleSubject(user, m->channel().toStdString() + suffix, m->topic().toStdString(), nickname);
}

void MyIrcSession::on_messageReceived(IrcMessage *message) {
	IrcPrivateMessage *m = (IrcPrivateMessage *) message;

	std::string target = m->target().toStdString();
	LOG4CXX_INFO(logger, user << ": Message from " << target);
	if (target.find("#") == 0) {
		bool flags = 0;
		std::string nickname = m->sender().name().toStdString();
		flags = correctNickname(nickname);
		np->handleMessage(user, target + suffix, m->message().toStdString(), nickname);
	}
	else {
		bool flags = 0;
		std::string nickname = m->sender().name().toStdString();
		flags = correctNickname(nickname);
		np->handleMessage(user, nickname, m->message().toStdString());
	}
}

void MyIrcSession::on_numericMessageReceived(IrcMessage *message) {
	IrcNumericMessage *m = (IrcNumericMessage *) message;
	switch (m->code()) {
		case 332:
			m_topicData = m->parameters().value(2).toStdString();
			break;
		case 333:
			 np->handleSubject(user, m->parameters().value(1).toStdString() + suffix, m_topicData, m->parameters().value(2).toStdString());
			break;
		case 353:
			QString channel = m->parameters().value(2);
			QStringList members = m->parameters().value(3).split(" ");

			for (int i = 0; i < members.size(); i++) {
				bool flags = 0;
				std::string nickname = members.at(i).toStdString();
				flags = correctNickname(nickname);
				m_modes[channel.toStdString() + nickname] = flags;
				np->handleParticipantChanged(user, nickname, channel.toStdString() + suffix,(int) flags, pbnetwork::STATUS_ONLINE);
			}
			break;
	}

	//qDebug() << "numeric message received:" << receiver() << origin << code << params;
}

void MyIrcSession::onMessageReceived(IrcMessage *message) {
	LOG4CXX_INFO(logger, user << ": " << message->toString().toStdString());
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
	}
}
