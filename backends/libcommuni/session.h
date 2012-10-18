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

using namespace Transport;

class IRCNetworkPlugin;

class MyIrcSession : public IrcSession
{
    Q_OBJECT

public:
	class AutoJoinChannel {
		public:
			AutoJoinChannel(const std::string &channel = "", const std::string &password = "") : m_channel(channel), m_password(password) {}
			virtual ~AutoJoinChannel() {}

			const std::string &getChannel() { return m_channel; }
			const std::string &getPassword() { return m_password; }
		private:
			std::string m_channel;
			std::string m_password;
	};

	typedef std::map<std::string, boost::shared_ptr<AutoJoinChannel> > AutoJoinMap;

	MyIrcSession(const std::string &user, IRCNetworkPlugin *np, const std::string &suffix = "", QObject* parent = 0);
	std::map<std::string, bool> m_modes;
	std::string suffix;
	int rooms;

	void addAutoJoinChannel(const std::string &channel, const std::string &password) {
		m_autoJoin[channel] = boost::make_shared<AutoJoinChannel>(channel, password);
	}

	void removeAutoJoinChannel(const std::string &channel) {
		m_autoJoin.erase(channel);
	}

	void setIdentify(const std::string &identify) {
		m_identify = identify;
	}

	const std::string  &getIdentify() {
		return m_identify;
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

protected:
	IRCNetworkPlugin *np;
	std::string user;
	std::string m_identify;
	AutoJoinMap m_autoJoin;
	std::string m_topicData;
	bool m_connected;
	std::list<std::string> m_rooms;
	std::list<std::string> m_names;
};

//class MyIrcBuffer : public Irc::Buffer
//{
//    Q_OBJECT

//public:
//	MyIrcBuffer(const QString& receiver, const std::string &user, NetworkPlugin *np, const std::string &suffix, Irc::Session* parent);
//	NetworkPlugin *np;
//	std::string user;
//	MyIrcSession *p;
//	std::string m_topicData;
//		std::string suffix;

//protected Q_SLOTS:
//    void on_receiverChanged(const QString& receiver);
//    void on_joined(const QString& origin);
//    void on_parted(const QString& origin, const QString& message);
//    void on_quit(const QString& origin, const QString& message);
//    void on_nickChanged(const QString& origin, const QString& nick);
//    void on_modeChanged(const QString& origin, const QString& mode, const QString& args);
//    void on_topicChanged(const QString& origin, const QString& topic);
//    void on_invited(const QString& origin, const QString& receiver, const QString& channel);
//    void on_kicked(const QString& origin, const QString& nick, const QString& message);
//    void on_messageReceived(const QString& origin, const QString& message, Irc::Buffer::MessageFlags flags);
//    void on_noticeReceived(const QString& origin, const QString& notice, Irc::Buffer::MessageFlags flags);
//    void on_ctcpRequestReceived(const QString& origin, const QString& request, Irc::Buffer::MessageFlags flags);
//    void on_ctcpReplyReceived(const QString& origin, const QString& reply, Irc::Buffer::MessageFlags flags);
//    void on_ctcpActionReceived(const QString& origin, const QString& action, Irc::Buffer::MessageFlags flags);
//    void on_numericMessageReceived(const QString& origin, uint code, const QStringList& params);
//    void on_unknownMessageReceived(const QString& origin, const QStringList& params);

//	bool correctNickname(std::string &nickname);
//};

#endif // SESSION_H
