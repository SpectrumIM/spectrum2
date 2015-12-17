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
	class AutoJoinChannel {
		public:
			AutoJoinChannel(const std::string &channel = "", const std::string &password = "", int awayCycle = 12) : m_channel(channel), m_password(password),
				m_awayCycle(awayCycle), m_currentAwayTick(0) {}
			virtual ~AutoJoinChannel() {}

			const std::string &getChannel() { return m_channel; }
			const std::string &getPassword() { return m_password; }
			bool shouldAskWho() {
				if (m_currentAwayTick == m_awayCycle) {
					m_currentAwayTick = 0;
					return true;
				}
				m_currentAwayTick++;
				return false;
			}

		private:
			std::string m_channel;
			std::string m_password;
			int m_awayCycle;
			int m_currentAwayTick;
	};

	class IRCBuddy {
		public:
			IRCBuddy(bool op = false, bool away = false) : m_op(op), m_away(away) {};

			void setOp(bool op) { m_op = op; }
			bool isOp() { return m_op; }
			void setAway(bool away) { m_away = away; }
			bool isAway() { return m_away; }
		
		private:
			bool m_op;
			bool m_away;
	};

	typedef std::map<std::string, boost::shared_ptr<AutoJoinChannel> > AutoJoinMap;
	typedef std::map<std::string, std::map<std::string, IRCBuddy> > IRCBuddyMap;

	MyIrcSession(const std::string &user, IRCNetworkPlugin *np, const std::string &suffix = "", QObject* parent = 0);
	virtual ~MyIrcSession();

	void addAutoJoinChannel(const std::string &channel, const std::string &password) {
		m_autoJoin[channel] = boost::make_shared<AutoJoinChannel>(channel, password, 12 + m_autoJoin.size());
	}

	void removeAutoJoinChannel(const std::string &channel) {
		m_autoJoin.erase(channel);
		removeIRCBuddies(channel);
	}

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

	bool hasIRCBuddy(const std::string &channel, const std::string &name) {
		return m_buddies[channel].find(name) != m_buddies[channel].end();
	}

	IRCBuddy &getIRCBuddy(const std::string &channel, const std::string &name) {
		return m_buddies[channel][name];
	}

	void removeIRCBuddy(const std::string &channel, const std::string &name) {
		m_buddies[channel].erase(name);
	}

	void removeIRCBuddies(const std::string &channel) {
		m_buddies.erase(channel);
	}

	bool correctNickname(std::string &nickname);

	void on_joined(IrcMessage *message);
	void on_parted(IrcMessage *message);
	void on_quit(IrcMessage *message);
	void on_nickChanged(IrcMessage *message);
	void on_modeChanged(IrcMessage *message);
	void on_topicChanged(IrcMessage *message);
	void on_messageReceived(IrcMessage *message);
	void on_numericMessageReceived(IrcMessage *message);
	void on_noticeMessageReceived(IrcMessage *message);

	std::string suffix;
	int rooms;

protected Q_SLOTS:
	void on_connected();
	void on_disconnected();
	void on_socketError(QAbstractSocket::SocketError error);

	void onMessageReceived(IrcMessage* message);
	void awayTimeout();

protected:
	IRCNetworkPlugin *np;
	std::string user;
	std::string m_identify;
	AutoJoinMap m_autoJoin;
	std::string m_topicData;
	bool m_connected;
	std::list<std::string> m_rooms;
	std::list<std::string> m_names;
	std::map<std::string, std::string> m_pms;
	IRCBuddyMap m_buddies;
	QTimer *m_awayTimer;
};

#endif // SESSION_H
