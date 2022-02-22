#pragma once

#include "Swiften/Swiften.h"

#include "transport/Config.h"
#include "transport/NetworkPlugin.h"

class Plugin : public Transport::NetworkPlugin {
	public:
		Plugin(Transport::Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port);

		// NetworkPlugin uses this method to send the data to networkplugin server
		void sendData(const std::string &string);

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password, const std::map<std::string, std::string> &settings);

		void handleLogoutRequest(const std::string &user, const std::string &legacyName);

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "", const std::string &id = "");

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups);

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups);

	private:
		// This method has to call handleDataRead with all received data from network plugin server
		void _handleDataRead(std::shared_ptr<Swift::SafeByteArray> data);

	private:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		std::shared_ptr<Swift::Connection> m_conn;
		Transport::Config *config;
};
