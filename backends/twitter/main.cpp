/*#include "transport/config.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include "transport/sqlite3backend.h"
#include "transport/mysqlbackend.h"
#include "transport/pqxxbackend.h"
#include "transport/storagebackend.h"

#include "Swiften/Swiften.h"
#include "unistd.h"
#include "signal.h"
#include "sys/wait.h"
#include "sys/signal.h"

#include <boost/algorithm/string.hpp>
#include <boost/signal.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "twitcurl.h"
#include "TwitterResponseParser.h"

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <queue>
#include <set>
#include <cstdio>

#include "ThreadPool.h"
#include "Requests/StatusUpdateRequest.h"
#include "Requests/DirectMessageRequest.h"
#include "Requests/TimelineRequest.h"
#include "Requests/FetchFriends.h"
#include "Requests/HelpMessageRequest.h"
#include "Requests/PINExchangeProcess.h"
#include "Requests/OAuthFlow.h"



using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;*/

#include "TwitterPlugin.h"

DEFINE_LOGGER(logger, "Twitter Backend");
//class TwitterPlugin; // The plugin

/*#define STR(x) (std::string("(") + x.from + ", " + x.to + ", " + x.message + ")")

class TwitterPlugin : public NetworkPlugin {
	private:
		struct Request;
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;

		TwitterPlugin(Config *config, Swift::SimpleEventLoop *loop, const std::string &host, int port) : NetworkPlugin() {
			this->config = config;

			if(CONFIG_HAS_KEY(config, "twitter.consumer_key") == false ||
			   CONFIG_HAS_KEY(config, "twitter.consumer_secret") == false) {
				LOG4CXX_ERROR(logger, "Couldn't find consumer key and/or secret. Please check config file.");
				exit(0);
			}
			consumerKey = CONFIG_STRING(config, "twitter.consumer_key");
			consumerSecret = CONFIG_STRING(config, "twitter.consumer_secret");
			OAUTH_KEY = "oauth_key";
			OAUTH_SECRET = "oauth_secret";

			m_factories = new Swift::BoostNetworkFactories(loop);
			m_conn = m_factories->getConnectionFactory()->createConnection();
			m_conn->onDataRead.connect(boost::bind(&TwitterPlugin::_handleDataRead, this, _1));
			m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));

			tp = new ThreadPool(loop_, 10);
				
			LOG4CXX_INFO(logger, "Starting the plugin.");
		}

		~TwitterPlugin() {
			delete storagebackend;
			std::map<std::string, twitCurl*>::iterator it;
			for(it = sessions.begin() ; it != sessions.end() ; it++) delete it->second;
			delete tp;
		}

		// Send data to NetworkPlugin server
		void sendData(const std::string &string) {
			m_conn->write(Swift::createSafeByteArray(string));
		}

		// Receive date from the NetworkPlugin server and invoke the appropirate payload handler (implement in the NetworkPlugin class)
		void _handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) {
			std::string d(data->begin(), data->end());
			handleDataRead(d);
		}
	
		// User trying to login into his twitter account
		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) {
			if(connectionState.count(user) && (connectionState[user] == NEW || 
						                        connectionState[user] == CONNECTED || 
												connectionState[user] == WAITING_FOR_PIN)) {
				LOG4CXX_INFO(logger, std::string("A session corresponding to ") + user + std::string(" is already active"))
				return;
			}
			
			LOG4CXX_INFO(logger, std::string("Received login request for ") + user)
			
			initUserSession(user, password);
			
			handleConnected(user);
			handleBuddyChanged(user, "twitter-account", "twitter", std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
			
			LOG4CXX_INFO(logger, "Querying database for usersettings of " << user)
			
			std::string key, secret;
			getUserOAuthKeyAndSecret(user, key, secret);

			if(key == "" || secret == "") {			
				LOG4CXX_INFO(logger, "Intiating OAuth Flow for user " << user)
				tp->runAsThread(new OAuthFlow(np, sessions[user], user, sessions[user]->getTwitterUsername()));
			} else {
				LOG4CXX_INFO(logger, user << " is already registerd. Using the stored oauth key and secret")
				LOG4CXX_INFO(logger, key << " " << secret)	
				pinExchangeComplete(user, key, secret);
			}
		}
		
		// User logging out
		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			delete sessions[user];
			sessions[user] = NULL;
			connectionState[user] = DISCONNECTED;
		}


		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") {
			
			if(legacyName == "twitter-account") {
				std::string cmd = message.substr(0, message.find(':'));
				std::string data = message.substr(message.find(':') + 1);
				
				handleMessage(user, "twitter-account", cmd + " " + data);

				if(cmd == "#pin") tp->runAsThread(new PINExchangeProcess(np, sessions[user], user, data));
				else if(cmd == "#help") tp->runAsThread(new HelpMessageRequest(np, user));
				else if(cmd[0] == '@') {
					std::string username = cmd.substr(1); 
					tp->runAsThread(new DirectMessageRequest(np, sessions[user], user, username, data));
				}
				else if(cmd == "#status") tp->runAsThread(new StatusUpdateRequest(np, sessions[user], user, data));
				else if(cmd == "#timeline") tp->runAsThread(new TimelineRequest(np, sessions[user], user));
				else if(cmd == "#friends") tp->runAsThread(new FetchFriends(np, sessions[user], user));
			}
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

		}


		bool getUserOAuthKeyAndSecret(const std::string user, std::string &key, std::string &secret) {
			boost::mutex::scoped_lock lock(dblock);
			
			UserInfo info;
			if(storagebackend->getUser(user, info) == false) {
				LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!")
				return false;
			}

			key="", secret=""; int type;
			storagebackend->getUserSetting((long)info.id, OAUTH_KEY, type, key);
			storagebackend->getUserSetting((long)info.id, OAUTH_SECRET, type, secret);
			return true;
		}

		bool storeUserOAuthKeyAndSecret(const std::string user, const std::string OAuthKey, const std::string OAuthSecret) {

			boost::mutex::scoped_lock lock(dblock);

			UserInfo info;
			if(storagebackend->getUser(user, info) == false) {
				LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!")
				return false;
			}

			storagebackend->updateUserSetting((long)info.id, OAUTH_KEY, OAuthKey);	
			storagebackend->updateUserSetting((long)info.id, OAUTH_SECRET, OAuthSecret);
			return true;
		}
		
		void initUserSession(const std::string user, const std::string password){
			boost::mutex::scoped_lock lock(userlock);

			std::string username = user.substr(0,user.find('@'));
			std::string passwd = password;
			LOG4CXX_INFO(logger, username + "  " + passwd)
	
			sessions[user] = new twitCurl();	
			if(CONFIG_HAS_KEY(config,"proxy.server")) {			
				std::string ip = CONFIG_STRING(config,"proxy.server");

				std::ostringstream out; 
				out << CONFIG_INT(config,"proxy.port");
				std::string port = out.str();

				std::string puser = CONFIG_STRING(config,"proxy.user");
				std::string ppasswd = CONFIG_STRING(config,"proxy.password");

				LOG4CXX_INFO(logger, ip << " " << port << " " << puser << " " << ppasswd)
				
				if(ip != "localhost" && port != "0") {
					sessions[user]->setProxyServerIp(ip);
		        	sessions[user]->setProxyServerPort(port);
		        	sessions[user]->setProxyUserName(puser);
		        	sessions[user]->setProxyPassword(ppasswd);
				}
			}

			connectionState[user] = NEW;			
			sessions[user]->setTwitterUsername(username);
			sessions[user]->setTwitterPassword(passwd); 
			sessions[user]->getOAuth().setConsumerKey(consumerKey);
			sessions[user]->getOAuth().setConsumerSecret(consumerSecret);
		}
		
		void OAuthFlowComplete(const std::string user, twitCurl *obj) {
			boost::mutex::scoped_lock lock(userlock);	

			delete sessions[user];
			sessions[user] = obj->clone();	
			connectionState[user] = WAITING_FOR_PIN;
		}	

		void pinExchangeComplete(const std::string user, const std::string OAuthAccessTokenKey, const std::string OAuthAccessTokenSecret) {
			boost::mutex::scoped_lock lock(userlock);	
				
			sessions[user]->getOAuth().setOAuthTokenKey( OAuthAccessTokenKey );
			sessions[user]->getOAuth().setOAuthTokenSecret( OAuthAccessTokenSecret );
			connectionState[user] = CONNECTED;
		}	

	private:
		enum status {NEW, WAITING_FOR_PIN, CONNECTED, DISCONNECTED};
		
		Config *config;

		std::string consumerKey;
		std::string consumerSecret;
		std::string OAUTH_KEY;
		std::string OAUTH_SECRET;

		boost::mutex dblock, userlock;

		ThreadPool *tp;
		std::map<std::string, twitCurl*> sessions;		
		std::map<std::string, status> connectionState;
};*/

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

	Config config;
	if (!config.load(argv[5])) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	Logging::initBackendLogging(&config);
	
	std::string error;
	StorageBackend *storagebackend;
	
	storagebackend = StorageBackend::createBackend(&config, error);
	if (storagebackend == NULL) {
		LOG4CXX_ERROR(logger, "Error creating StorageBackend! " << error)
		return -2;
	}

	else if (!storagebackend->connect()) {
		LOG4CXX_ERROR(logger, "Can't connect to database!")
		return -1;
	}

	Swift::SimpleEventLoop eventLoop;
	loop_ = &eventLoop;
	np = new TwitterPlugin(&config, &eventLoop, storagebackend, host, port);
	loop_->run();

	return 0;
}
