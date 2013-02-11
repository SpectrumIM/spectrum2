/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#ifndef SESSION_H
#define SESSION_H

#include <IrcSession>
#include <transport/networkplugin.h>
#include "Swiften/Swiften.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <QTimer>

using namespace Transport;

class IRCNetworkPlugin;

class MyIrcSession : public IrcSession
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
	std::string suffix;
	int rooms;

	void addAutoJoinChannel(const std::string &channel, const std::string &password) {
		m_autoJoin[channel] = boost::make_shared<AutoJoinChannel>(channel, password, 12 + m_autoJoin.size());
	}

	void removeAutoJoinChannel(const std::string &channel) {
		m_autoJoin.erase(channel);
		removeIRCBuddies(channel);
	}

	void addPM(const std::string &name, const std::string &room) {
		m_pms[name] = room;
	}

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
