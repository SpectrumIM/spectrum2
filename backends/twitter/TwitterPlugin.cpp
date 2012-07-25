#include "TwitterPlugin.h"
#include "Requests/StatusUpdateRequest.h"
#include "Requests/DirectMessageRequest.h"
#include "Requests/TimelineRequest.h"
#include "Requests/FetchFriends.h"
#include "Requests/HelpMessageRequest.h"
#include "Requests/PINExchangeProcess.h"
#include "Requests/OAuthFlow.h"
#include "Requests/CreateFriendRequest.h"
#include "Requests/DestroyFriendRequest.h"
#include "Requests/RetweetRequest.h"
#include "Requests/ProfileImageRequest.h"
#include "Swiften/StringCodecs/Hexify.h"

DEFINE_LOGGER(logger, "Twitter Backend");

TwitterPlugin *np = NULL;
Swift::SimpleEventLoop *loop_; // Event Loop

#define abs(x) ((x)<0?-(x):(x))
//#define SHA(x) (convertToChars(Swift::byteArrayToString(Swift::SHA1::getHash(Swift::createByteArray((x))))))
#define SHA(x) (Swift::Hexify::hexify(Swift::SHA1::getHash(Swift::createByteArray((x)))))

//static std::string convertToChars(std::string s)
//{
//	std::string ret = "";
//	std::string tab[] = {"0","1","2","3","4","5","6","7",
//						 "8","9","A","B","C","D","E","F"};
//	for(int i = 0 ; i < s.size() ; i++)
//		ret = tab[s[i]>>4&15] + tab[s[i]&15] + ret;
//	return ret;
//}

//Compares two +ve intergers 'a' and 'b' represented as strings 
static int cmp(std::string a, std::string b)
{
	int diff = abs((int)a.size() - (int)b.size());
	if(a.size() < b.size()) a = std::string(diff,'0') + a;
	else b = std::string(diff,'0') + b;
	
	if(a == b) return 0;
	if(a < b) return -1;
	return 1;
}


TwitterPlugin::TwitterPlugin(Config *config, Swift::SimpleEventLoop *loop, StorageBackend *storagebackend, const std::string &host, int port) : NetworkPlugin() 
{
	this->config = config;
	this->storagebackend = storagebackend;

	if(CONFIG_HAS_KEY(config, "twitter.consumer_key") == false ||
	   CONFIG_HAS_KEY(config, "twitter.consumer_secret") == false) {
		LOG4CXX_ERROR(logger, "Couldn't find consumer key and/or secret. Please check config file.");
		exit(0);
	}
	
	adminLegacyName = "twitter-account"; 
	adminChatRoom = "twitter-chatroom"; 
	adminNickName = "twitter"; 
	adminAlias = "twitter";

	consumerKey = CONFIG_STRING(config, "twitter.consumer_key");
	consumerSecret = CONFIG_STRING(config, "twitter.consumer_secret");

	OAUTH_KEY = "oauth_key";
	OAUTH_SECRET = "oauth_secret";
	MODE = "mode";

	m_factories = new Swift::BoostNetworkFactories(loop);
	m_conn = m_factories->getConnectionFactory()->createConnection();
	m_conn->onDataRead.connect(boost::bind(&TwitterPlugin::_handleDataRead, this, _1));
	m_conn->connect(Swift::HostAddressPort(Swift::HostAddress(host), port));

	tp = new ThreadPool(loop_, 10);
		
	tweet_timer = m_factories->getTimerFactory()->createTimer(60000);
	message_timer = m_factories->getTimerFactory()->createTimer(60000);

	tweet_timer->onTick.connect(boost::bind(&TwitterPlugin::pollForTweets, this));
	message_timer->onTick.connect(boost::bind(&TwitterPlugin::pollForDirectMessages, this));

	tweet_timer->start();
	message_timer->start();
	
	LOG4CXX_INFO(logger, "Starting the plugin.");
}

TwitterPlugin::~TwitterPlugin() 
{
	delete storagebackend;
	std::set<std::string>::iterator it;
	for(it = onlineUsers.begin() ; it != onlineUsers.end() ; it++) delete userdb[*it].sessions;
	delete tp;
}

// Send data to NetworkPlugin server
void TwitterPlugin::sendData(const std::string &string) 
{
	m_conn->write(Swift::createSafeByteArray(string));
}

// Receive date from the NetworkPlugin server and invoke the appropirate payload handler (implement in the NetworkPlugin class)
void TwitterPlugin::_handleDataRead(boost::shared_ptr<Swift::SafeByteArray> data) 
{
	std::string d(data->begin(), data->end());
	handleDataRead(d);
}

// User trying to login into his twitter account
void TwitterPlugin::handleLoginRequest(const std::string &user, const std::string &legacyName, const std::string &password) 
{
	if(userdb.count(user) && (userdb[user].connectionState == NEW || 
										userdb[user].connectionState == CONNECTED || 
										userdb[user].connectionState == WAITING_FOR_PIN)) {
		LOG4CXX_INFO(logger, std::string("A session corresponding to ") + user + std::string(" is already active"))
		return;
	}
	
	LOG4CXX_INFO(logger, std::string("Received login request for ") + user)	
	initUserSession(user, password);
	handleConnected(user);
	
	LOG4CXX_INFO(logger, user << ": Adding Buddy " << adminLegacyName << " " << adminAlias)
	handleBuddyChanged(user, adminLegacyName, adminAlias, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
	userdb[user].nickName = "";
	
	LOG4CXX_INFO(logger, "Querying database for usersettings of " << user)
	std::string key, secret;
	getUserOAuthKeyAndSecret(user, key, secret);

	if(key == "" || secret == "") {			
		LOG4CXX_INFO(logger, "Intiating OAuth Flow for user " << user)
		setTwitterMode(user, 0);
		tp->runAsThread(new OAuthFlow(np, userdb[user].sessions, user, userdb[user].sessions->getTwitterUsername()));
	} else {
		LOG4CXX_INFO(logger, user << " is already registerd. Using the stored oauth key and secret")
		LOG4CXX_INFO(logger, key << " " << secret)	
		pinExchangeComplete(user, key, secret);
	}
}

// User logging out
void TwitterPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) 
{
	if(onlineUsers.count(user)) {
		delete userdb[user].sessions;
		userdb[user].sessions = NULL;
		userdb[user].connectionState = DISCONNECTED;
		onlineUsers.erase(user);
	}
}

// User joining a Chatroom
void TwitterPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password)
{
	if(room == adminChatRoom) {	
		LOG4CXX_INFO(logger, "Received Join Twitter room request for " << user)

		setTwitterMode(user, 2);
		handleParticipantChanged(user, adminNickName, room, 0, pbnetwork::STATUS_ONLINE);
		userdb[user].nickName = nickname;
		handleMessage(user, adminChatRoom, "Connected to Twitter room! Populating your followers list", adminNickName);
		tp->runAsThread(new FetchFriends(userdb[user].sessions, user,
										 boost::bind(&TwitterPlugin::populateRoster, this, _1, _2, _3, _4)));
	} else {
		setTwitterMode(user, 0);
		LOG4CXX_ERROR(logger, "Couldn't connect to chatroom - " << room <<"! Try twitter-chatroom as the chatroom to access Twitter account")
		handleMessage(user, adminLegacyName, "Couldn't connect to chatroom! Try twitter-chatroom as the chatroom to access Twitter account");
	}	
}

// User leaving a Chatroom
void TwitterPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room)
{
	if(room == adminChatRoom && onlineUsers.count(user)) {
		LOG4CXX_INFO(logger, "Leaving chatroom! Switching back to default mode 0")
		setTwitterMode(user, 0);
		handleBuddyChanged(user, adminLegacyName, adminAlias, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
	}
}

// Messages to be sent to Twitter 
void TwitterPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml) 
{

	LOG4CXX_INFO(logger, "Received " << user << " --> " << legacyName << " - " << message)
	
	if(legacyName == adminLegacyName || legacyName == adminChatRoom)  {
		std::string cmd = "", data = "";
	 
		/** Parsing the message - Assuming message format to be <cmd>[ ]*<data>**/	
		int i;
		for(i=0 ; i<message.size() && message[i] != ' '; i++) cmd += message[i];
		while(i<message.size() && message[i] == ' ') i++;
		data = message.substr(i);
		/***********************************************************************/
		
		if(cmd == "#pin") 
			tp->runAsThread(new PINExchangeProcess(np, userdb[user].sessions, user, data));
		else if(cmd == "#help") 
			tp->runAsThread(new HelpMessageRequest(user, boost::bind(&TwitterPlugin::helpMessageResponse, this, _1, _2)));
		else if(cmd[0] == '@') {
			std::string username = cmd.substr(1); 
			tp->runAsThread(new DirectMessageRequest(userdb[user].sessions, user, username, data,
												     boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
		}
		else if(cmd == "#status") 
			tp->runAsThread(new StatusUpdateRequest(userdb[user].sessions, user, data,
														boost::bind(&TwitterPlugin::statusUpdateResponse, this, _1, _2)));
		else if(cmd == "#timeline") 
			tp->runAsThread(new TimelineRequest(userdb[user].sessions, user, data, "",
														boost::bind(&TwitterPlugin::displayTweets, this, _1, _2, _3, _4)));
		else if(cmd == "#friends") 
			tp->runAsThread(new FetchFriends(userdb[user].sessions, user,
													   boost::bind(&TwitterPlugin::displayFriendlist, this, _1, _2, _3, _4)));
		else if(cmd == "#follow") 
			tp->runAsThread(new CreateFriendRequest(userdb[user].sessions, user, data.substr(0,data.find('@')),
													   boost::bind(&TwitterPlugin::createFriendResponse, this, _1, _2, _3, _4)));
		else if(cmd == "#unfollow") 
			tp->runAsThread(new DestroyFriendRequest(userdb[user].sessions, user, data.substr(0,data.find('@')),
													   boost::bind(&TwitterPlugin::deleteFriendResponse, this, _1, _2, _3)));
		else if(cmd == "#retweet") 
			tp->runAsThread(new RetweetRequest(userdb[user].sessions, user, data,
													   boost::bind(&TwitterPlugin::RetweetResponse, this, _1, _2)));
		else if(cmd == "#mode") {
			int m = 0;
			m = atoi(data.c_str());
			mode prevm = userdb[user].twitterMode;

			if((mode)m == userdb[user].twitterMode) return; //If same as current mode return
			if(m < 0 || m > 2) { // Invalid modes
				handleMessage(user, adminLegacyName, std::string("Error! Unknown mode ") + data + ". Allowed values 0,1,2." );
				return;
			}

			setTwitterMode(user, m);
			if((userdb[user].twitterMode == SINGLECONTACT || userdb[user].twitterMode == CHATROOM) && prevm == MULTIPLECONTACT) clearRoster(user);
			else if(userdb[user].twitterMode == MULTIPLECONTACT) 
				tp->runAsThread(new FetchFriends(userdb[user].sessions, user, boost::bind(&TwitterPlugin::populateRoster, this, _1, _2, _3, _4)));

			handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
								std::string("Changed mode to ") + data, userdb[user].twitterMode == CHATROOM ? adminNickName : "");

			LOG4CXX_INFO(logger, user << ": Changed mode to " << data  << " <" << (userdb[user].twitterMode == CHATROOM ? adminNickName : "") << ">" )
		}

		else if(userdb[user].twitterMode == CHATROOM) {
			std::string buddy = message.substr(0, message.find(":"));
			if(userdb[user].buddies.count(buddy) == 0) {
				tp->runAsThread(new StatusUpdateRequest(userdb[user].sessions, user, message,
														boost::bind(&TwitterPlugin::statusUpdateResponse, this, _1, _2)));
			} else {
				data = message.substr(message.find(":")+1); //Can parse better??:P
				tp->runAsThread(new DirectMessageRequest(userdb[user].sessions, user, buddy, data,
												 		 boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
			}
		}
		else handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
				                 "Unknown command! Type #help for a list of available commands.", userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	} 

	else {	
		std::string buddy;
		if(userdb[user].twitterMode == CHATROOM) buddy = legacyName.substr(legacyName.find("/") + 1);
		if(legacyName != "twitter") {
			tp->runAsThread(new DirectMessageRequest(userdb[user].sessions, user, buddy, message,
												 boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
		}
	}
}

void TwitterPlugin::handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) 
{
	if(userdb[user].connectionState != CONNECTED) {
		LOG4CXX_ERROR(logger, user << " is not connected to twitter!")
		return;
	}

	LOG4CXX_INFO(logger, user << " - Adding Twitter contact " << buddyName)
	tp->runAsThread(new CreateFriendRequest(userdb[user].sessions, user, buddyName, 
											boost::bind(&TwitterPlugin::createFriendResponse, this, _1, _2, _3, _4)));
}

void TwitterPlugin::handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) 
{
	if(userdb[user].connectionState != CONNECTED) {
		LOG4CXX_ERROR(logger, user << " is not connected to twitter!")
		return;
	}
	
	LOG4CXX_INFO(logger, user << " - Removing Twitter contact " << buddyName)
	tp->runAsThread(new DestroyFriendRequest(userdb[user].sessions, user, buddyName, 
											 boost::bind(&TwitterPlugin::deleteFriendResponse, this, _1, _2, _3)));
}

void TwitterPlugin::handleVCardRequest(const std::string &user, const std::string &legacyName, unsigned int id)
{
	if(userdb[user].connectionState != CONNECTED) {
		LOG4CXX_ERROR(logger, user << " is not connected to twitter!")
		return;
	}
	
	LOG4CXX_INFO(logger, user << " - VCardRequest for " << legacyName << ", " << userdb[user].buddiesInfo[legacyName].getProfileImgURL())

	if(getTwitterMode(user) != SINGLECONTACT && userdb[user].buddies.count(legacyName) 
		&& userdb[user].buddiesInfo[legacyName].getProfileImgURL().length()) {
		if(userdb[user].buddiesImgs.count(legacyName) == 0) {
			tp->runAsThread(new ProfileImageRequest(config, user, legacyName, userdb[user].buddiesInfo[legacyName].getProfileImgURL(), id,
													boost::bind(&TwitterPlugin::profileImageResponse, this, _1, _2, _3, _4, _5)));
		}
		handleVCard(user, id, legacyName, legacyName, "", userdb[user].buddiesImgs[legacyName]);
	}
}

void TwitterPlugin::pollForTweets()
{
	boost::mutex::scoped_lock lock(userlock);
	std::set<std::string>::iterator it = onlineUsers.begin();
	while(it != onlineUsers.end()) {
		std::string user = *it;
		tp->runAsThread(new TimelineRequest(userdb[user].sessions, user, "", userdb[user].mostRecentTweetID,
											boost::bind(&TwitterPlugin::displayTweets, this, _1, _2, _3, _4)));
		it++;
	}
	tweet_timer->start();
}

void TwitterPlugin::pollForDirectMessages()
{
	boost::mutex::scoped_lock lock(userlock);
	std::set<std::string>::iterator it = onlineUsers.begin();
	while(it != onlineUsers.end()) {
		std::string user = *it;
		tp->runAsThread(new DirectMessageRequest(userdb[user].sessions, user, "", userdb[user].mostRecentDirectMessageID,
											boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
		it++;
	}
	message_timer->start();
}


bool TwitterPlugin::getUserOAuthKeyAndSecret(const std::string user, std::string &key, std::string &secret) 
{
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

int TwitterPlugin::getTwitterMode(const std::string user) 
{
	boost::mutex::scoped_lock lock(dblock);
	
	UserInfo info;
	if(storagebackend->getUser(user, info) == false) {
		LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!")
		return -1;
	}

	int type; int m;
	std::string s_m;
	storagebackend->getUserSetting((long)info.id, MODE, type, s_m);
	if(s_m == "") {
		s_m  = "0";
		storagebackend->updateUserSetting((long)info.id, MODE, s_m);
	}
	m = atoi(s_m.c_str());
	return m;
}

bool TwitterPlugin::setTwitterMode(const std::string user, int m) 
{
	boost::mutex::scoped_lock lock(dblock);
	
	UserInfo info;
	if(storagebackend->getUser(user, info) == false) {
		LOG4CXX_ERROR(logger, "Didn't find entry for " << user << " in the database!")
		return false;
	}

	if(m < 0 || m > 2) {
		LOG4CXX_ERROR(logger, "Unknown mode " << m <<". Using default mode 0")
		m = 0;
	}

	userdb[user].twitterMode = (mode)m;

	//int type;
	std::string s_m = std::string(1,m+'0');
	LOG4CXX_ERROR(logger, "Storing mode " << m <<" for user " << user)
	storagebackend->updateUserSetting((long)info.id, MODE, s_m);
	return true;
}

bool TwitterPlugin::storeUserOAuthKeyAndSecret(const std::string user, const std::string OAuthKey, const std::string OAuthSecret) 
{

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

void TwitterPlugin::initUserSession(const std::string user, const std::string password)
{
	boost::mutex::scoped_lock lock(userlock);

	std::string username = user.substr(0,user.find('@'));
	std::string passwd = password;
	LOG4CXX_INFO(logger, username + "  " + passwd)

	userdb[user].sessions = new twitCurl();	
	if(CONFIG_HAS_KEY(config,"proxy.server")) {			
		std::string ip = CONFIG_STRING(config,"proxy.server");

		std::ostringstream out; 
		out << CONFIG_INT(config,"proxy.port");
		std::string port = out.str();

		std::string puser = CONFIG_STRING(config,"proxy.user");
		std::string ppasswd = CONFIG_STRING(config,"proxy.password");

		LOG4CXX_INFO(logger, ip << " " << port << " " << puser << " " << ppasswd)
		
		if(ip != "localhost" && port != "0") {
			userdb[user].sessions->setProxyServerIp(ip);
			userdb[user].sessions->setProxyServerPort(port);
			userdb[user].sessions->setProxyUserName(puser);
			userdb[user].sessions->setProxyPassword(ppasswd);
		}
	}

	userdb[user].connectionState = NEW;			
	userdb[user].sessions->setTwitterUsername(username);
	userdb[user].sessions->setTwitterPassword(passwd); 
	userdb[user].sessions->getOAuth().setConsumerKey(consumerKey);
	userdb[user].sessions->getOAuth().setConsumerSecret(consumerSecret);
}

void TwitterPlugin::OAuthFlowComplete(const std::string user, twitCurl *obj) 
{
	boost::mutex::scoped_lock lock(userlock);	

	delete userdb[user].sessions;
	userdb[user].sessions = obj->clone();	
	userdb[user].connectionState = WAITING_FOR_PIN;
}	

void TwitterPlugin::pinExchangeComplete(const std::string user, const std::string OAuthAccessTokenKey, const std::string OAuthAccessTokenSecret) 
{
	boost::mutex::scoped_lock lock(userlock);	
		
	userdb[user].sessions->getOAuth().setOAuthTokenKey( OAuthAccessTokenKey );
	userdb[user].sessions->getOAuth().setOAuthTokenSecret( OAuthAccessTokenSecret );
	userdb[user].connectionState = CONNECTED;
	userdb[user].twitterMode = (mode)getTwitterMode(user);
	
	if(userdb[user].twitterMode == MULTIPLECONTACT) {
		tp->runAsThread(new FetchFriends(userdb[user].sessions, user, boost::bind(&TwitterPlugin::populateRoster, this, _1, _2, _3, _4)));
	}

	onlineUsers.insert(user);
	userdb[user].mostRecentTweetID = "";
	userdb[user].mostRecentDirectMessageID = "";
}	

void TwitterPlugin::updateLastTweetID(const std::string user, const std::string ID)
{
	boost::mutex::scoped_lock lock(userlock);	
	userdb[user].mostRecentTweetID = ID;
}

std::string TwitterPlugin::getMostRecentTweetID(const std::string user)
{
	boost::mutex::scoped_lock lock(userlock);	
	std::string ID = "-1";
	if(onlineUsers.count(user)) ID = userdb[user].mostRecentTweetID;
	return ID;
}

void TwitterPlugin::updateLastDMID(const std::string user, const std::string ID)
{
	boost::mutex::scoped_lock lock(userlock);	
	userdb[user].mostRecentDirectMessageID = ID;
}

std::string TwitterPlugin::getMostRecentDMID(const std::string user)
{
	boost::mutex::scoped_lock lock(userlock);	
	std::string ID = "";
	if(onlineUsers.count(user)) ID = userdb[user].mostRecentDirectMessageID;
	return ID;
}

/************************************** Twitter response functions **********************************/
void TwitterPlugin::statusUpdateResponse(std::string &user, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	} else {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							"Status Update successful", userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	}
}

void TwitterPlugin::helpMessageResponse(std::string &user, std::string &msg)
{
	handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
						msg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
}

void TwitterPlugin::clearRoster(const std::string user)
{
	if(userdb[user].buddies.size() == 0) return;
	std::set<std::string>::iterator it = userdb[user].buddies.begin();
	while(it != userdb[user].buddies.end()) {
		handleBuddyRemoved(user, *it);
		it++;
	}
	userdb[user].buddies.clear();
}

void TwitterPlugin::populateRoster(std::string &user, std::vector<User> &friends, std::vector<std::string> &friendAvatars, std::string &errMsg) 
{
	if(errMsg.length() == 0) 
	{
		for(int i=0 ; i<friends.size() ; i++) {
			userdb[user].buddies.insert(friends[i].getScreenName());
			userdb[user].buddiesInfo[friends[i].getScreenName()] = friends[i];
			userdb[user].buddiesImgs[friends[i].getScreenName()] = friendAvatars[i];
			
			if(userdb[user].twitterMode == MULTIPLECONTACT) {
				std::string lastTweet = friends[i].getLastStatus().getTweet();
				//LOG4CXX_INFO(logger, user << " - " << SHA(friendAvatars[i]))
				handleBuddyChanged(user, friends[i].getScreenName(), friends[i].getUserName(), std::vector<std::string>(), 
								   pbnetwork::STATUS_ONLINE, lastTweet, SHA(friendAvatars[i]));
			}
			else if(userdb[user].twitterMode == CHATROOM)
				handleParticipantChanged(user, friends[i].getScreenName(), adminChatRoom, 0, pbnetwork::STATUS_ONLINE);
			
			/*handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							   	friends[i].getScreenName() + " - " + friends[i].getLastStatus().getTweet(), 
								userdb[user].twitterMode == CHATROOM ? adminNickName : "");*/
		}
	} else handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							   std::string("Error populating roster - ") + errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");	

	if(userdb[user].twitterMode == CHATROOM) handleParticipantChanged(user, userdb[user].nickName, adminChatRoom, 0, pbnetwork::STATUS_ONLINE);
}

void TwitterPlugin::displayFriendlist(std::string &user, std::vector<User> &friends, std::vector<std::string> &friendAvatars, std::string &errMsg)
{
	if(errMsg.length() == 0) 
	{
		std::string userlist = "\n***************USER LIST****************\n";
		for(int i=0 ; i < friends.size() ; i++) {
			userlist += " - " + friends[i].getUserName() + " (" + friends[i].getScreenName() + ")\n";
		}	
		userlist += "***************************************\n";
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							userlist, userdb[user].twitterMode == CHATROOM ? adminNickName : "");	
	} else handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName, 
							   errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");	
 
}

void TwitterPlugin::displayTweets(std::string &user, std::string &userRequested, std::vector<Status> &tweets , std::string &errMsg)
{
	if(errMsg.length() == 0) {
		
		std::string timeline = "";
		std::map<std::string, int> lastTweet;
		std::map<std::string, int>::iterator it;

		for(int i=0 ; i<tweets.size() ; i++) {
			if(userdb[user].twitterMode != CHATROOM) {
				timeline += " - " + tweets[i].getUserData().getScreenName() + ": " + tweets[i].getTweet() + " (MsgId: " + tweets[i].getID() + ")\n";

				std::string scrname = tweets[i].getUserData().getScreenName();
				if(lastTweet.count(scrname) == 0 || cmp(tweets[lastTweet[scrname]].getID(), tweets[i].getID()) <= 0) lastTweet[scrname] = i;

			} else {
				handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
									tweets[i].getTweet() + " (MsgId: " + tweets[i].getID() + ")", tweets[i].getUserData().getScreenName());
			}
		}
		
		if(userdb[user].twitterMode == MULTIPLECONTACT) {
			//Set as status user's last tweet
			for(it=lastTweet.begin() ; it!=lastTweet.end() ; it++) {
				int t =  it->second;
				handleBuddyChanged(user, tweets[t].getUserData().getScreenName(), tweets[t].getUserData().getUserName(), 
								   std::vector<std::string>(), pbnetwork::STATUS_ONLINE, tweets[t].getTweet());
			}
		}

		if((userRequested == "" || userRequested == user) && tweets.size()) {
			std::string tweetID = getMostRecentTweetID(user);
			if(tweetID != tweets[0].getID()) updateLastTweetID(user, tweets[0].getID());
		}

		if(timeline.length()) handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
												  timeline, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	} else handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							   errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");	
}

void TwitterPlugin::directMessageResponse(std::string &user, std::string &username, std::vector<DirectMessage> &messages, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							std::string("Error while sending direct message! - ") + errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");	
		return;
	}

	if(username != "") {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
						   "Message delivered!", userdb[user].twitterMode == CHATROOM ? adminNickName : "");	
		return;
	}
	
	if(!messages.size()) return;
	
	if(userdb[user].twitterMode == SINGLECONTACT) {

		std::string msglist = "";
		std::string msgID = getMostRecentDMID(user);
		std::string maxID = msgID;
		
		for(int i=0 ; i < messages.size() ; i++) {
			if(cmp(msgID, messages[i].getID()) == -1) {
				msglist += " - " + messages[i].getSenderData().getScreenName() + ": " + messages[i].getMessage() + "\n";
				if(cmp(maxID, messages[i].getID()) == -1) maxID = messages[i].getID();
			}
		}	

		if(msglist.length()) handleMessage(user, adminLegacyName, msglist, "");	
		updateLastDMID(user, maxID);

	} else {
		
		std::string msgID = getMostRecentDMID(user);
		std::string maxID = msgID;

		for(int i=0 ; i < messages.size() ; i++) {
			if(cmp(msgID, messages[i].getID()) == -1) {
				if(userdb[user].twitterMode == MULTIPLECONTACT)
					handleMessage(user, messages[i].getSenderData().getScreenName(), messages[i].getMessage(), "");
				else
					handleMessage(user, adminChatRoom, messages[i].getMessage() + " - <Direct Message>", messages[i].getSenderData().getScreenName());
				if(cmp(maxID, messages[i].getID()) == -1) maxID = messages[i].getID();
			}
		}	
		
		if(maxID == getMostRecentDMID(user)) LOG4CXX_INFO(logger, "No new direct messages for " << user)
		updateLastDMID(user, maxID);
	}
}

void TwitterPlugin::createFriendResponse(std::string &user, User &frnd, std::string &img, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
		return;
	}

	handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
						std::string("You are now following ") + frnd.getScreenName(), userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	
	userdb[user].buddies.insert(frnd.getScreenName());
	userdb[user].buddiesInfo[frnd.getScreenName()] = frnd;
	userdb[user].buddiesImgs[frnd.getScreenName()] = img;
	
	LOG4CXX_INFO(logger, user << " - " << frnd.getScreenName() << ", " << frnd.getProfileImgURL())
	if(userdb[user].twitterMode == MULTIPLECONTACT) {
		handleBuddyChanged(user, frnd.getScreenName(), frnd.getUserName(), std::vector<std::string>(), pbnetwork::STATUS_ONLINE, "", SHA(img));
	} else if(userdb[user].twitterMode == CHATROOM) {
		handleParticipantChanged(user, frnd.getScreenName(), adminChatRoom, 0, pbnetwork::STATUS_ONLINE);
	}
}

void TwitterPlugin::deleteFriendResponse(std::string &user, User &frnd, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName, 
							errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
		return;
	} 
	
	LOG4CXX_INFO(logger, user << " - " << frnd.getScreenName() << ", " << frnd.getProfileImgURL())
	userdb[user].buddies.erase(frnd.getScreenName());
	
	handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
						std::string("You are not following ") + frnd.getScreenName() + " anymore", userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	
	if (userdb[user].twitterMode == CHATROOM) {
		handleParticipantChanged(user, frnd.getScreenName(), adminLegacyName, 0, pbnetwork::STATUS_NONE);
	}
	
	if(userdb[user].twitterMode == MULTIPLECONTACT) {
		handleBuddyRemoved(user, frnd.getScreenName());
	} 
}


void TwitterPlugin::RetweetResponse(std::string &user, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	} else {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							"Retweet successful", userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	}
}

void TwitterPlugin::profileImageResponse(std::string &user, std::string &buddy, std::string &img, unsigned int reqID, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, userdb[user].twitterMode == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, userdb[user].twitterMode == CHATROOM ? adminNickName : "");
	} else {
		LOG4CXX_INFO(logger, user << " - Sending VCard for " << buddy)
		handleVCard(user, reqID, buddy, buddy, "", img);
	}
}
