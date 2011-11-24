
#pragma once

#include "transport/config.h"
#include "transport/networkplugin.h"
#include "session.h"
#include <QtCore>
#include <QtNetwork>
#include "Swiften/EventLoop/Qt/QtEventLoop.h"
#include "ircnetworkplugin.h"


class SingleIRCNetworkPlugin : public QObject, public NetworkPlugin {
	Q_OBJECT

	public:
		SingleIRCNetworkPlugin(Config *config, Swift::QtEventLoop *loop, const std::string &host, int port);

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password);

		void handleLogoutRequest(const std::string &user, const std::string &legacyName);

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &/*xhtml*/);

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password);

		void handleLeaveRoomRequest(const std::string &user, const std::string &room);

		std::map<std::string, MyIrcSession *> m_sessions;

	public slots:
		void readData();
		void sendData(const std::string &string);

	private:
		Config *config;
		QTcpSocket *m_socket;
		std::string m_server;
		std::string m_identify;
};
