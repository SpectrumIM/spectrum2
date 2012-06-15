#ifndef TWITTER_PLUGIN
#define TWITTER_PLUGIN

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

#include "ThreadPool.h"

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace Transport;


#define STR(x) (std::string("(") + x.from + ", " + x.to + ", " + x.message + ")")
class TwitterPlugin;
extern TwitterPlugin *np;
extern Swift::SimpleEventLoop *loop_; // Event Loop

class TwitterPlugin : public NetworkPlugin {
	public:
		Swift::BoostNetworkFactories *m_factories;
		Swift::BoostIOServiceThread m_boostIOServiceThread;
		boost::shared_ptr<Swift::Connection> m_conn;
		Swift::Timer::ref tweet_timer;
		Swift::Timer::ref message_timer;
		StorageBackend *storagebackend;

		TwitterPlugin(Config *config, Swift::SimpleEventLoop *loop, StorageBackend *storagebackend, const std::string &host, int port);
		~TwitterPlugin();

		// Send data to NetworkPlugin server
		void sendData(const std::string &string);

		// Receive date from the NetworkPlugin server and invoke the appropirate payload handler (implement in the NetworkPlugin class)
		void _handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data);
	
		// User trying to login into his twitter account
		void handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password);
		
		// User logging out
		void handleLogoutRequest(const std::string &user, const std::string &legacyName);

		void handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml = "");

		void handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups);

		void handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups);
		
		void pollForTweets();

		void pollForDirectMessages();
		
		bool getUserOAuthKeyAndSecret(const std::string user, std::string &key, std::string &secret);
		
		bool storeUserOAuthKeyAndSecret(const std::string user, const std::string OAuthKey, const std::string OAuthSecret);
		
		void initUserSession(const std::string user, const std::string password);
		
		void OAuthFlowComplete(const std::string user, twitCurl *obj);
		
		void pinExchangeComplete(const std::string user, const std::string OAuthAccessTokenKey, const std::string OAuthAccessTokenSecret);
		
		void updateLastTweetID(const std::string user, const std::string ID);

		std::string getMostRecentTweetID(const std::string user);

		void updateLastDMID(const std::string user, const std::string ID);
		
		std::string getMostRecentDMID(const std::string user);

		/****************** Twitter Response handlers **************************************/
		void populateRoster(std::string &user, std::vector<User> &friends, std::string &errMsg);
		void displayFriendlist(std::string &user, std::vector<User> &friends, std::string &errMsg);
		void displayTweets(std::string &user, std::string &userRequested, std::vector<Status> &tweets , std::string &errMsg);
		void directMessageResponse(std::string &user, std::vector<DirectMessage> &messages, std::string &errMsg);
		void createFriendResponse(std::string &user, std::string &frnd, std::string &errMsg);
		void deleteFriendResponse(std::string &user, std::string &frnd, std::string &errMsg);
		void RetweetResponse(std::string &user, std::string &errMsg);
		/***********************************************************************************/

	private:
		enum status {NEW, WAITING_FOR_PIN, CONNECTED, DISCONNECTED};
		enum mode {SINGLECONTACT, MULTIPLECONTACT, CHATROOM};

		Config *config;

		std::string consumerKey;
		std::string consumerSecret;
		std::string OAUTH_KEY;
		std::string OAUTH_SECRET;

		boost::mutex dblock, userlock;

		ThreadPool *tp;
		std::map<std::string, twitCurl*> sessions;		
		std::map<std::string, status> connectionState;
		std::map<std::string, std::string> mostRecentTweetID;
		std::map<std::string, std::string> mostRecentDirectMessageID;
		std::set<std::string> onlineUsers;
		mode twitterMode;
};

#endif
