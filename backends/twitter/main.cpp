#include "transport/config.h"
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
#include "userdb.h"

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;

DEFINE_LOGGER(logger, "Twitter Backend");
Swift::SimpleEventLoop *loop_; // Event Loop
class TwitterPlugin; // The plugin
TwitterPlugin * np = NULL;
StorageBackend *storagebackend;

#define STR(x) (std::string("(") + x.from + ", " + x.to + ", " + x.message + ")")

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
			onDispatchRequest.connect(boost::bind(&TwitterPlugin::requestDispatcher, this, _1, _2));
			
			activeThreadCount = 0;
			MAX_THREADS  = 50;
			
			LOG4CXX_INFO(logger, "Starting the plugin.");
		}

		~TwitterPlugin() {
			delete storagebackend;
			std::map<std::string, twitCurl*>::iterator it;
			for(it = sessions.begin() ; it != sessions.end() ; it++) delete it->second;
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
			
			std::string username = user.substr(0,user.find('@'));
			std::string passwd = password;
			LOG4CXX_INFO(logger, username + "  " + passwd)

			sessions[user] = new twitCurl();
			handleConnected(user);
			handleBuddyChanged(user, "twitter-account", "twitter", std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
			
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
			
			LOG4CXX_INFO(logger, "Querying database for usersettings of " << user)
			
			UserInfo info;
			storagebackend->getUser(user, info);

			std::string key="", secret="";
			int type;
			storagebackend->getUserSetting((long)info.id, OAUTH_KEY, type, key);
			storagebackend->getUserSetting((long)info.id, OAUTH_SECRET, type, secret);

			if(key == "" || secret == "") {			
				LOG4CXX_INFO(logger, "Intiating oauthflow for user " << user)

				std::string authUrl;
				if (sessions[user]->oAuthRequestToken( authUrl ) == false ) {
					LOG4CXX_ERROR(logger, "Error creating twitter authorization url!");
					handleLogoutRequest(user, username);
					return;
				}
				handleMessage(user, "twitter-account", std::string("Please visit the following link and authorize this application: ") + authUrl);
				handleMessage(user, "twitter-account", std::string("Please reply with the PIN provided by twitter. Prefix the pin with 'pin:'. Ex. 'pin: 1234'"));
				connectionState[user] = WAITING_FOR_PIN;	
			} else {
				LOG4CXX_INFO(logger, user << " is already registerd. Using the stored oauth key and secret")
				LOG4CXX_INFO(logger, key << " " << secret)

				sessions[user]->getOAuth().setOAuthTokenKey( key );
				sessions[user]->getOAuth().setOAuthTokenSecret( secret );
				connectionState[user] = CONNECTED;
			}
		}
		
		// User logging out
		void handleLogoutRequest(const std::string &user, const std::string &legacyName) {
			delete sessions[user];
			sessions[user] = NULL;
			connectionState[user] = DISCONNECTED;
		}

		void handlePINExchange(const std::string &user, std::string &data) {
			sessions[user]->getOAuth().setOAuthPin( data );
			if (sessions[user]->oAuthAccessToken() == false) {
				LOG4CXX_ERROR(logger, user << ": Error while exchanging PIN for Access Token!")
				handleLogoutRequest(user, "");
				return;
			}
			
			std::string OAuthAccessTokenKey, OAuthAccessTokenSecret;
			sessions[user]->getOAuth().getOAuthTokenKey( OAuthAccessTokenKey );
			sessions[user]->getOAuth().getOAuthTokenSecret( OAuthAccessTokenSecret );

			UserInfo info;
			if(storagebackend->getUser(user, info) == false) {
				LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!")
				handleLogoutRequest(user, "");
				return;
			}

			storagebackend->updateUserSetting((long)info.id, OAUTH_KEY, OAuthAccessTokenKey);	
			storagebackend->updateUserSetting((long)info.id, OAUTH_SECRET, OAuthAccessTokenSecret);	

			connectionState[user] = CONNECTED;
			LOG4CXX_INFO(logger, user << ": Sent PIN " << data << " and obtained Access Token");
		}

		void printHelpMessage(const std::string &user) {
			std::string helpMsg = "";
			helpMsg = helpMsg
				    + "\nHELP\n"
					+ "#status:<your status> - Update your status\n"
					+ "#timeline - Retrieve your timeline\n"
					+ "@<username>:<message> - Send a directed message to the user <username>\n"
					+ "#help - print this help message\n";

			handleMessage(user, "twitter-account", helpMsg);
		}

		void handleDirectMessage(const std::string &user, std::string &username, std::string &data) {
			if(sessions[user]->directMessageSend(username, data, false) == false) {
				LOG4CXX_ERROR(logger, user << ": Error while sending directed message to user " << username );
				return;
			}

			LOG4CXX_INFO(logger, user << ": Sending " << data << " to " << username)

			std::string replyMsg;
			sessions[user]->getLastWebResponse( replyMsg );
			LOG4CXX_INFO(logger, replyMsg);
		}

		void handleStatusUpdate(const std::string &user, std::string &data) {
			if(connectionState[user] != CONNECTED) {
				LOG4CXX_ERROR(logger, "Trying to update status for " << user << " when not connected!");
				return;
			}

			std::string replyMsg; 
			if( sessions[user]->statusUpdate( data ) ) {
				replyMsg = "";
				while(replyMsg.length() == 0) {
					sessions[user]->getLastWebResponse( replyMsg );
				}
				LOG4CXX_INFO(logger, user << ": twitCurl:statusUpdate web response: " << replyMsg );
			} else {
				sessions[user]->getLastCurlError( replyMsg );
				LOG4CXX_INFO(logger, user << ": twitCurl::statusUpdate error: " << replyMsg );
			}
			LOG4CXX_INFO(logger, "Updated status for " << user << ": " << data);
		}

		void fetchTimeline(const std::string &user) {
			if(connectionState[user] != CONNECTED) {
				LOG4CXX_ERROR(logger, "Trying to fetch timeline for " << user << " when not connected!");
				return;
			}
			
			std::string replyMsg = ""; 
			if( sessions[user]->timelineHomeGet()) {
				
				while(replyMsg.length() == 0) {
					sessions[user]->getLastWebResponse( replyMsg );
				}

				LOG4CXX_INFO(logger, user << ": twitCurl::timeline web response: " << replyMsg.length() << " " << replyMsg << "\n" );
				
				std::vector<Status> tweets = getTimeline(replyMsg);
				std::string timeline = "\n";
				for(int i=0 ; i<tweets.size() ; i++) {
					timeline += tweets[i].getTweet() + "\n";
				}

				handleMessage(user, "twitter-account", timeline);

			} else {
				sessions[user]->getLastCurlError( replyMsg );
				LOG4CXX_INFO(logger, user << ": twitCurl::timeline error: " << replyMsg );
			}
		}

		void fetchFriends(const std::string &user) {
			if(connectionState[user] != CONNECTED) {
				LOG4CXX_ERROR(logger, "Trying to fetch friends of " << user << " when not connected!");
				return;
			}
			
			std::string replyMsg = ""; 
			if( sessions[user]->friendsIdsGet(sessions[user]->getTwitterUsername())) {
				
				while(replyMsg.length() == 0) {
					sessions[user]->getLastWebResponse( replyMsg );
				}

				LOG4CXX_INFO(logger, user << ": twitCurl::friendsIdsGet web response: " << replyMsg.length() << " " << replyMsg << "\n" );

				std::vector<std::string> IDs = getIDs( replyMsg );
				/*for(int i=0 ; i<IDs.size() ; i++) {
					//LOG4CXX_INFO(logger, "ID #" << i+1 << ": " << IDs[i]);
					
				}*/
				sessions[user]->userLookup(IDs, true);
				sessions[user]->getLastWebResponse( replyMsg );
				LOG4CXX_INFO(logger, user << ": twitCurl::UserLookUp web response: " << replyMsg.length() << " " << replyMsg << "\n" );

				std::vector<User> users = getUsers( replyMsg );
				
				std::string userlist = "\n***************USER LIST****************\n";
				for(int i=0 ; i < users.size() ; i++) {
					userlist += "*)" + users[i].getUserName() + " (" + users[i].getScreenName() + ")\n";
				}	
				userlist += "***************************************\n";	
				handleMessage(user, "twitter-account", userlist);

			} else {
				sessions[user]->getLastCurlError( replyMsg );
				LOG4CXX_INFO(logger, user << ": twitCurl::friendsIdsGet error: " << replyMsg );
			}
			
		}

		void spawnThreadForRequest(Request r) {
			std::string &user = r.from;
			std::string &legacyName = r.to;
		    std::string &message = r.message;

			LOG4CXX_INFO(logger, "Sending message from " << user << " to " << legacyName << ".");

			if(legacyName == "twitter-account") {
				std::string cmd = message.substr(0, message.find(':'));
				std::string data = message.substr(message.find(':') + 1);
				
				handleMessage(user, "twitter-account", cmd + " " + data);

				if(cmd == "#pin") handlePINExchange(user, data);
				else if(cmd == "#help") printHelpMessage(user);
				else if(cmd[0] == '@') {std::string username = cmd.substr(1); handleDirectMessage(user, username, data);}
				else if(cmd == "#status") handleStatusUpdate(user, data);
				else if(cmd == "#timeline") fetchTimeline(user);
				else if(cmd == "#friends") fetchFriends(user);
			}
			updateActiveThreadCount(-1);
			onDispatchRequest(r, false);	
		}
		
		/*
		 * usersBeingServed - set of users being served at present
		 * usersToServe - queue of users who have requests pending in their request queue and are yet to be served; Each user appears only once here.
		 */
		void requestDispatcher(Request r, bool incoming) {

			criticalRegion.lock();

			if(incoming) {
				std::string user = r.from;
				if(getActiveThreadCount() < MAX_THREADS && usersBeingServed.count(user) == false) {
					updateActiveThreadCount(1);
					boost::thread(&TwitterPlugin::spawnThreadForRequest, this, r);
					usersBeingServed.insert(user);
					LOG4CXX_INFO(logger, user << ": Sending request " << STR(r) << " to twitter")
				} else {
					requests[user].push(r);
					LOG4CXX_INFO(logger, user << " is already being served! Adding " << STR(r) << " to request queue");
					if (!usersBeingServed.count(user)) {
						usersToServe.push(user);
					}
				}
			} else {
				usersBeingServed.erase(r.from);
				if(requests[r.from].size()) usersToServe.push(r.from);
				while(getActiveThreadCount() < MAX_THREADS && !usersToServe.empty()) {
					std::string user = usersToServe.front(); usersToServe.pop();
					Request s = requests[user].front(); requests[user].pop();
					updateActiveThreadCount(1);
					boost::thread(&TwitterPlugin::spawnThreadForRequest, this, s);
					usersBeingServed.insert(user);
					LOG4CXX_INFO(logger, user << ": Sending request " << STR(s) << " to twitter")
				} 
			}

			criticalRegion.unlock();
		}


		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "") {
			
			Request r;
			r.from = user;
			r.to = legacyName;
			r.message = message;
			LOG4CXX_INFO(logger, user << "Dispatching request " << STR(r))
			onDispatchRequest(r,true);
			//requestDispatcher(r, true);

			/*if(legacyName == "twitter-account") {
				std::string cmd = message.substr(0, message.find(':'));
				std::string data = message.substr(message.find(':') + 1);
				
				handleMessage(user, "twitter-account", cmd + " " + data);

				if(cmd == "#pin") handlePINExchange(user, data);
				else if(cmd == "#help") printHelpMessage(user);
				else if(cmd[0] == '@') {std::string username = cmd.substr(1); handleDirectMessage(user, username, data);}
				else if(cmd == "#status") handleStatusUpdate(user, data);
				else if(cmd == "#timeline") fetchTimeline(user);
				else if(cmd == "#friends") fetchFriends(user);
			}*/
		}

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) {
			LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
			handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
		}

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) {

		}

		int getActiveThreadCount() {
			int res;
			threadLock.lock();
			res = activeThreadCount;
			threadLock.unlock();
			return res;
		}

		void updateActiveThreadCount(int k) {
			threadLock.lock();
			activeThreadCount+=k;
			threadLock.unlock();
		}

	private:
		enum status {NEW, WAITING_FOR_PIN, CONNECTED, DISCONNECTED};
		struct Request {
			std::string from;
			std::string to;
			std::string message;
		};

		Config *config;
		std::string consumerKey;
		std::string consumerSecret;
		std::string OAUTH_KEY;
		std::string OAUTH_SECRET;

		int activeThreadCount;
		int MAX_THREADS;
		
		boost::mutex criticalRegion;
		boost::mutex threadLock;

		std::map<std::string, twitCurl*> sessions;
		std::map<std::string, std::queue<Request> > requests;
		
		std::queue<std::string> usersToServe;
		std::set<std::string> usersBeingServed;

		
		std::map<std::string, status> connectionState;

		boost::signal< void (Request, bool) > onDispatchRequest;
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

	Config config;
	if (!config.load(argv[5])) {
		std::cerr << "Can't open " << argv[1] << " configuration file.\n";
		return 1;
	}

	Logging::initBackendLogging(&config);
	
	std::string error;
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
	np = new TwitterPlugin(&config, &eventLoop, host, port);
	loop_->run();

	return 0;
}
