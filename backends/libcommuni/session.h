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

#ifndef SESSION_H
#define SESSION_H

#ifndef Q_MOC_RUN
#include <IrcConnection>
#include <IrcBufferModel>
#include <IrcBuffer>
#include <IrcUserModel>
#include <transport/NetworkPlugin.h>
#include "Swiften/Swiften.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <QTimer>
#endif

using namespace Transport;

class IRCNetworkPlugin;

class MyIrcSession : public IrcConnection
{
    Q_OBJECT

public:
	MyIrcSession(const std::string &user, IRCNetworkPlugin *np, const std::string &suffix = "", QObject* parent = 0);
	virtual ~MyIrcSession();

	void createBufferModel();

	// We are sending PM message. On XMPP side, user is sending PM using the particular channel,
	// for example #room@irc.freenode.org/hanzz. On IRC side, we are forwarding this message
	// just to "hanzz". Therefore we have to somewhere store, that message from "hanzz" should
	// be mapped to #room@irc.freenode.org/hanzz.
	void addPM(const std::string &name, const std::string &room);

	void setIdentify(const std::string &identify) {
		m_identify = identify;
	}

	const std::string  &getIdentify() {
		return m_identify;
	}

	bool hasIrcUser(const std::string &channel, const std::string &name);

	void correctNickname(std::string &nick);
	IrcUser *getIrcUser(IrcBuffer *buffer, IrcMessage *message);
	IrcUser *getIrcUser(IrcBuffer *buffer, std::string &nick);

	void sendWhoisCommand(const std::string &channel, const std::string &to);
	void sendMessageToFrontend(const std::string &channel, const std::string &nickname, const std::string &msg);
	void sendUserToFrontend(IrcUser *user, pbnetwork::StatusType statusType, const std::string &statusMessage = "", const std::string &newNick = "");

	void on_joined(IrcMessage *message);
	void on_parted(IrcMessage *message);
	void on_quit(IrcMessage *message);
	void on_nickChanged(IrcMessage *message);
	void on_topicChanged(IrcMessage *message);
	void on_messageReceived(IrcMessage *message);
	void on_numericMessageReceived(IrcMessage *message);
	void on_noticeMessageReceived(IrcMessage *message);
	void on_whoisMessageReceived(IrcMessage *message);
	void on_namesMessageReceived(IrcMessage *message);

	int rooms;

protected Q_SLOTS:
	void on_connected();
	void on_disconnected();
	void on_socketError(QAbstractSocket::SocketError error);

	void onBufferAdded(IrcBuffer* buffer);
	void onBufferRemoved(IrcBuffer* buffer);

	void onIrcUserAdded(IrcUser *user);
	void onIrcUserChanged(const QString &);
	void onIrcUserChanged(bool);
	void onIrcUserRemoved(IrcUser *user);

	void onMessageReceived(IrcMessage* message);
	void awayTimeout();


protected:
	IRCNetworkPlugin *m_np;
	IrcBufferModel *m_bufferModel;
	std::string m_user;
	std::string m_identify;
	std::string m_topicData;
	bool m_connected;
	std::list<std::string> m_rooms;
	std::list<std::string> m_names;
	std::map<std::string, std::string> m_pms;
	QTimer *m_awayTimer;
	std::string m_suffix;
	std::map<std::string, std::string> m_whois;
	QHash<IrcBuffer*, IrcUserModel*> m_userModels;
};

#endif // SESSION_H
