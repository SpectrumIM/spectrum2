/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/sqlite3backend.h"
#include "transport/mysqlbackend.h"
#include "transport/pqxxbackend.h"
#include "transport/storagebackend.h"
#include "Swiften/Swiften.h"
#include <boost/filesystem.hpp>
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"
#include <fstream>
#include <streambuf>

Swift::SimpleEventLoop *loop_;

#include "log4cxx/logger.h"
#include "log4cxx/consoleappender.h"
#include "log4cxx/patternlayout.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/helpers/properties.h"
#include "log4cxx/helpers/fileinputstream.h"
#include "log4cxx/helpers/transcoder.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::filesystem;

using namespace boost::program_options;
using namespace Transport;

using namespace log4cxx;

static LoggerPtr logger = log4cxx::Logger::getLogger("SMSNetworkPlugin");

#define INTERNAL_USER "/sms@backend@internal@user"

class SMSNetworkPlugin;
SMSNetworkPlugin * np = NULL;
StorageBackend *storageBackend;

class SMSNetworkPlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;
		Swift::Timer::ref m_timer;
		int m_internalUser;

		SMSNetworkPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;
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
			if (config->getUnregistered().find("backend.incoming_dir") != config->getUnregistered().end()) {
				dir = config->getUnregistered().find("backend.incoming_dir")->second;
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
			std::string data = "To: " + to + "\n";
			data += "\n";
			data += msg;

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

		void _handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
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

			BOOST_FOREACH(BuddyInfo &b, roster) {
				handleBuddyChanged(user, b.legacyName, b.alias, b.groups, pbnetwork::STATUS_ONLINE);
			}

			np->handleConnected(user);
//			std::vector<std::string> groups;
//			groups.push_back("ZCode");
//			np->handleBuddyChanged(user, "zcode", "ZCode", groups, pbnetwork::STATUS_ONLINE);
		}

		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
		}

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") {
			std::string n = legacyName;
			if (n.find("+") == 0) {
				n = n.substr(1);
			}

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

	boost::program_options::options_description desc("Usage: spectrum [OPTIONS] <config_file.cfg>\nAllowed options");
	desc.add_options()
		("host,h", value<std::string>(&host), "host")
		("port,p", value<int>(&port), "port")
		;
	try
	{
		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);
	}
	catch (std::runtime_error& e)
	{
		std::cout << desc << "\n";
		exit(1);
	}
	catch (...)
	{
		std::cout << desc << "\n";
		exit(1);
	}


	if (argc < 5) {
		return 1;
	}

// 	QStringList channels;
// 	for (int i = 3; i < argc; ++i)
// 	{
// 		channels.append(argv[i]);
// 	}
// 
// 	MyIrcSession session;
// 	session.setNick(argv[2]);
// 	session.setAutoJoinChannels(channels);
// 	session.connectToServer(argv[1], 6667);

	Config config;
	if (!config.load(argv[5])) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	if (CONFIG_STRING(&config, "logging.backend_config").empty()) {
		LoggerPtr root = log4cxx::Logger::getRootLogger();
#ifndef _MSC_VER
		root->addAppender(new ConsoleAppender(new PatternLayout("%d %-5p %c: %m%n")));
#else
		root->addAppender(new ConsoleAppender(new PatternLayout(L"%d %-5p %c: %m%n")));
#endif
	}
	else {
		log4cxx::helpers::Properties p;
		log4cxx::helpers::FileInputStream *istream = new log4cxx::helpers::FileInputStream(CONFIG_STRING(&config, "logging.backend_config"));
		p.load(istream);
		LogString pid, jid;
		log4cxx::helpers::Transcoder::decode(boost::lexical_cast<std::string>(getpid()), pid);
		log4cxx::helpers::Transcoder::decode(CONFIG_STRING(&config, "service.jid"), jid);
#ifdef _MSC_VER
		p.setProperty(L"pid", pid);
		p.setProperty(L"jid", jid);
#else
		p.setProperty("pid", pid);
		p.setProperty("jid", jid);
#endif
		log4cxx::PropertyConfigurator::configure(p);
	}

#ifdef WITH_SQLITE
	if (CONFIG_STRING(&config, "database.type") == "sqlite3") {
		storageBackend = new SQLite3Backend(&config);
		if (!storageBackend->connect()) {
			std::cerr << "Can't connect to database. Check the log to find out the reason.\n";
			return -1;
		}
	}
#else
	if (CONFIG_STRING(&config, "database.type") == "sqlite3") {
		std::cerr << "Spectrum2 is not compiled with mysql backend.\n";
		return -2;
	}
#endif

#ifdef WITH_MYSQL
	if (CONFIG_STRING(&config, "database.type") == "mysql") {
		storageBackend = new MySQLBackend(&config);
		if (!storageBackend->connect()) {
			std::cerr << "Can't connect to database. Check the log to find out the reason.\n";
			return -1;
		}
	}
#else
	if (CONFIG_STRING(&config, "database.type") == "mysql") {
		std::cerr << "Spectrum2 is not compiled with mysql backend.\n";
		return -2;
	}
#endif

#ifdef WITH_PQXX
	if (CONFIG_STRING(&config, "database.type") == "pqxx") {
		storageBackend = new PQXXBackend(&config);
		if (!storageBackend->connect()) {
			std::cerr << "Can't connect to database. Check the log to find out the reason.\n";
			return -1;
		}
	}
#else
	if (CONFIG_STRING(&config, "database.type") == "pqxx") {
		std::cerr << "Spectrum2 is not compiled with pqxx backend.\n";
		return -2;
	}
#endif

	if (CONFIG_STRING(&config, "database.type") != "mysql" && CONFIG_STRING(&config, "database.type") != "sqlite3"
		&& CONFIG_STRING(&config, "database.type") != "pqxx" && CONFIG_STRING(&config, "database.type") != "none") {
		std::cerr << "Unknown storage backend " << CONFIG_STRING(&config, "database.type") << "\n";
		return -2;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new SMSNetworkPlugin(&config, &eventLoop, host, port);
	loop_->run();

	return 0;
}
