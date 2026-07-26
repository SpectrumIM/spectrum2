/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "transport/Config.h"
#include "transport/Logging.h"
#include "transport/BoostNetworkPlugin.h"
#include <boost/filesystem.hpp>
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#include <fstream>
#include <streambuf>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::filesystem;
using namespace boost::program_options;

using namespace Transport;

DEFINE_LOGGER(logger, "SMSNetworkPlugin");

class SMSNetworkPlugin;
SMSNetworkPlugin * np = NULL;

class SMSNetworkPlugin : public BoostNetworkPlugin {
	public:
		std::shared_ptr<boost::asio::deadline_timer> m_timer;
		// Maps phone number -> XMPP user JID for SMS routing
		std::map<std::string, std::string> m_numberToUser;

		SMSNetworkPlugin(Config *config, const std::string &host, int port)
			: BoostNetworkPlugin(config, host, port) {

			m_timer = std::make_shared<boost::asio::deadline_timer>(io_context, boost::posix_time::seconds(5));
			m_timer->async_wait(boost::bind(&SMSNetworkPlugin::handleSMSDir, this));
		}


		void handleSMS(const std::string &sms) {
			LOG4CXX_INFO(logger, "Handling SMS " << sms << ".");
			std::ifstream t(sms.c_str());
			std::string str;

			t.seekg(0, std::ios::end);
			str.reserve(t.tellg());
			t.seekg(0, std::ios::beg);

			str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

			std::string from = "";
			std::string msg = "";
			while (str.find("\n") != std::string::npos) {
				std::string line = str.substr(0, str.find("\n"));
				if (line.find("From: ") == 0) {
					from = line.substr(strlen("From: "));
				}
				else if (line.empty()) {
					msg = str.substr(1);
					break;
				}
				str = str.substr(str.find("\n") + 1);
			}

			std::string to = m_numberToUser[from];

			if (to.empty()) {
				LOG4CXX_WARN(logger, "Received SMS from " << from << ", but this number is not associated with any XMPP user.");
				return;
			}

			LOG4CXX_INFO(logger, "Forwarding SMS from " << from << " to " << to << ".");
			handleMessage(to, from, msg);
		}

		void handleSMSDir() {
			std::string dir = "/var/spool/sms/incoming/";
			if (CONFIG_HAS_KEY(config, "backend.incoming_dir")) {
				dir = CONFIG_STRING(config, "backend.incoming_dir");
			}
			LOG4CXX_INFO(logger, "Checking directory " << dir << " for incoming SMS.");

			path p(dir);
			directory_iterator end_itr;
			for (directory_iterator itr(p); itr != end_itr; ++itr) {

				try {
					if (is_regular(itr->path())) {
						handleSMS(itr->path().string());
						remove(itr->path());
					}
				}
				catch (const filesystem_error& ex) {
					LOG4CXX_ERROR(logger, "Error when removing the SMS: " << ex.what() << ".");
				}
			}

			m_timer->expires_from_now(boost::posix_time::seconds(5));
			m_timer->async_wait(boost::bind(&SMSNetworkPlugin::handleSMSDir, this));
		}

		void sendSMS(const std::string &to, const std::string &msg) {
			std::string data = "To: " + to + "\n";
			data += "\n";
			data += msg;

			// generate random string here...
			std::string bucket = "abcdefghijklmnopqrstuvwxyz";
			std::string uuid;
			for (int i = 0; i < 10; i++) {
				uuid += bucket[rand() % bucket.size()];
			}
			std::ofstream myfile;
			myfile.open (std::string("/var/spool/sms/outgoing/spectrum." + uuid).c_str());
			myfile << data;
			myfile.close();
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password, const std::map<std::string, std::string> &settings) {
			LOG4CXX_INFO(logger, "User " << user << " logged in");

			// Send available presence for all numbers that route to this user
			for (std::map<std::string, std::string>::iterator it = m_numberToUser.begin(); it != m_numberToUser.end(); ++it) {
				if (it->second == user) {
					std::vector<std::string> groups;
					handleBuddyChanged(user, it->first, it->second, groups, pbnetwork::STATUS_ONLINE);
				}
			}

			np->handleConnected(user);
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "", const std::string &id = "") {
			// Remove trailing +, because smstools doesn't use it in "From: " field for received messages.
			std::string n = legacyName;
			if (n.find("+") == 0) {
				n = n.substr(1);
			}

			// Store routing info: phone number -> XMPP user
			m_numberToUser[n] = user;

			LOG4CXX_INFO(logger, "Sending SMS from " << user << " to " << n << ".");
			sendSMS(n, message);
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
			// Restore routing info: buddyName = phone number, alias contains user JID info
			if (!alias.empty()) {
				m_numberToUser[buddyName] = alias;
			}
			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {
			m_numberToUser.erase(buddyName);
		}
};

static void spectrum_sigchld_handler(int sig)
{
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WNOHANG);
	} while (pid != 0 && pid != (pid_t)-1);

	if ((pid == (pid_t) - 1) && (errno != ECHILD)) {
		char errmsg[BUFSIZ];
		snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
		perror(errmsg);
	}
}


int main (int argc, char* argv[]) {
	std::string host;
	int port;

	if (signal(SIGCHLD, spectrum_sigchld_handler) == SIG_ERR) {
		std::cout << "SIGCHLD handler can't be set\n";
		return -1;
	}

	std::string error;
	Config *cfg = Config::createFromArgs(argc, argv, error, host, port);
	if (cfg == NULL) {
		std::cerr << error;
		return 1;
	}

	Logging::initBackendLogging(cfg);

	np = new SMSNetworkPlugin(cfg, host, port);
	np->run();

	return 0;
}
