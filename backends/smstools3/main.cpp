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
#include "transport/NetworkPlugin.h"
#include "transport/SQLite3Backend.h"
#include "transport/MySQLBackend.h"
#include "transport/PQXXBackend.h"
#include "transport/StorageBackend.h"
#include "Swiften/Swiften.h"
#include <boost/filesystem.hpp>
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#include <fstream>
#include <streambuf>

Swift::SimpleEventLoop *loop_;

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::filesystem;

using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "SMSNetworkPlugin");

#define INTERNAL_USER "/sms@backend@internal@user"

class SMSNetworkPlugin;
SMSNetworkPlugin * np = NULL;

class SMSNetworkPlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		std::shared_ptr<Swift::Connection> m_conn;
		Swift::Timer::ref m_timer;
		int m_internalUser;
		StorageBackend *storageBackend;

		SMSNetworkPlugin(Config *config, Swift::SimpleEventLoop *loop, StorageBackend *storagebackend, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
			this->storageBackend = storagebackend;
			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&SMSNetworkPlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));
// 			m_conn->onConnectFinished.connect(boost::bind(&FrotzNetworkPlugin::_handleConnected, this, _1));
// 			m_conn->onDisconnected.connect(boost::bind(&FrotzNetworkPlugin::handleDisconnected, this));

			LOG4CXX_INFO(logger, "Starting the plugin.");

			m_timer = m_factories->getTimerFactory()->createTimer(5000);
			m_timer->onTick.connect(boost::bind(&SMSNetworkPlugin::handleSMSDir, this));
			m_timer->start();

			// We're reusing our database model here. Buddies of user with JID INTERNAL_USER are there
			// to match received GSM messages from number N with the XMPP users who sent message to number N.
			// BuddyName = GSM number
			// Alias = XMPP user JID to which the messages from this number is sent to.
			// TODO: This should be per Modem!!!
			UserInfo info;
			info.jid = INTERNAL_USER;
			info.password = "";
			storageBackend->setUser(info);
			storageBackend->getUser(INTERNAL_USER, info);
			m_internalUser = info.id;
		}


		void handleSMS(const std::string &sms) {
			LOG4CXX_INFO(logger, "Handling SMS " << sms << ".")
			std::ifstream t(sms.c_str());
			std::string str;

			t.seekg(0, std::ios::end);
			str.reserve(t.tellg());
			t.seekg(0, std::ios::beg);

			str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

			std::string from = "";
			std::string msg = "";
			while(str.find("\n") != std::string::npos) {
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

			std::list<BuddyInfo> roster;
			storageBackend->getBuddies(m_internalUser, roster);

			std::string to;
			BOOST_FOREACH(BuddyInfo &b, roster) {
				if (b.legacyName == from) {
					to = b.alias;
				}
			}

			if (to.empty()) {
				LOG4CXX_WARN(logger, "Received SMS from " << from << ", but this number is not associated with any XMPP user.");
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
			m_timer->start();
		}

		void sendSMS(const std::string &to, const std::string &msg) {
			// TODO: Probably 
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

		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		void _handleDataRead(std::shared_ptr<Swift::SafeByteArray> data) {
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}

		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			UserInfo info;
			if (!storageBackend->getUser(user, info)) {
				handleDisconnected(user, 0, "Not registered user.");
				return;
			}
			std::list<BuddyInfo> roster;
			storageBackend->getBuddies(info.id, roster);

			// Send available presence to every number in the roster.
			BOOST_FOREACH(BuddyInfo &b, roster) {
				handleBuddyChanged(user, b.legacyName, b.alias, b.groups, pbnetwork::STATUS_ONLINE);
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

			// Create GSM Number - XMPP user pair to match the potential response and send it to the proper JID.
			BuddyInfo info;
			info.legacyName = n;
			info.alias = user;
			info.id = -1;
			info.subscription = "both";
			info.flags = 0;
			storageBackend->addBuddy(m_internalUser, info);

			LOG4CXX_INFO(logger, "Sending SMS from " << user << " to " << n << ".");
			sendSMS(n, message);
		}

		void handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password) {
		}

		void handleLeaveRoomRequest(const std::string &user, const std::string &room) {
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

		}


	private:
		
		Config *config;
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

	StorageBackend *storageBackend = StorageBackend::createBackend(cfg, error);
	if (storageBackend == NULL) {
		if (!error.empty()) {
			std::cerr << error << "\n";
			return -2;
		}
	}
	else if (!storageBackend->connect()) {
		std::cerr << "Can't connect to database. Check the log to find out the reason.\n";
		return -1;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new SMSNetworkPlugin(cfg, &eventLoop, storageBackend, host, port);
	loop_->run();

	return 0;
}
