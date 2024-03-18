#pragma once

#include <memory>

#include <boost/asio.hpp>

#include "transport/Config.h"
#include "transport/BoostNetworkPlugin.h"

class Plugin : public Transport::BoostNetworkPlugin {
	public:
		Plugin(Transport::Config *config, const std::string &host, int port);

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password, const std::map<std::string, std::string> &settings);

		void handleLogoutRequest(const std::string &user, const std::string &legacyName);

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "", const std::string &id = "");

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups);

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups);
};
