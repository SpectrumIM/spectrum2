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

using namespace Transport;

class MyIrcSession : public IrcSession
{
    Q_OBJECT

public:
	MyIrcSession(const std::string &user, NetworkPlugin *np, const std::string &suffix = "", QObject* parent = 0);
	std::map<std::string, bool> m_modes;
	std::string suffix;
	int rooms;

	void addAutoJoinChannel(const std::string &channel) {
		m_autoJoin.push_back(channel);
	}

	void removeAutoJoinChannel(const std::string &channel) {
		m_autoJoin.remove(channel);
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

	void onMessageReceived(IrcMessage* message);

protected:
	NetworkPlugin *np;
	std::string user;
	std::string m_identify;
	std::list<std::string> m_autoJoin;
	std::string m_topicData;
	bool m_connected;
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
