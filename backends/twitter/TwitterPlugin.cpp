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

DEFINE_LOGGER(logger, "Twitter Backend");

TwitterPlugin *np = NULL;
Swift::SimpleEventLoop *loop_; // Event Loop

#define abs(x) ((x)<0?-(x):(x))
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
	
	//twitterMode = SINGLECONTACT;
	//if(CONFIG_HAS_KEY(config, "twitter.mode") == false) {
	//	LOG4CXX_INFO(logger, "Using default single contact mode!");
	//} else twitterMode = (mode)CONFIG_INT(config, "twitter.mode");


	adminLegacyName = "twitter-account"; 
	adminChatRoom = "twitter-chatroom"; 
	adminNickName = "twitter"; 
	adminAlias = "twitter";
	//if(twitterMode == CHATROOM) adminNickName = "twitter";

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

	//tweet_timer->start();
	//message_timer->start();
	
	LOG4CXX_INFO(logger, "Starting the plugin.");
}

TwitterPlugin::~TwitterPlugin() 
{
	delete storagebackend;
	std::map<std::string, twitCurl*>::iterator it;
	for(it = sessions.begin() ; it != sessions.end() ; it++) delete it->second;
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
	if(connectionState.count(user) && (connectionState[user] == NEW || 
										connectionState[user] == CONNECTED || 
										connectionState[user] == WAITING_FOR_PIN)) {
		LOG4CXX_INFO(logger, std::string("A session corresponding to ") + user + std::string(" is already active"))
		return;
	}
	
	LOG4CXX_INFO(logger, std::string("Received login request for ") + user)
	
	initUserSession(user, password);
	
	handleConnected(user);
	
	LOG4CXX_INFO(logger, user << ": Adding Buddy " << adminLegacyName << " " << adminAlias)
	handleBuddyChanged(user, adminLegacyName, adminAlias, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
	nickName[user] = "";
	//setTwitterMode(user, 0);
	
	LOG4CXX_INFO(logger, "Querying database for usersettings of " << user)
	
	std::string key, secret;
	getUserOAuthKeyAndSecret(user, key, secret);

	if(key == "" || secret == "") {			
		LOG4CXX_INFO(logger, "Intiating OAuth Flow for user " << user)
		setTwitterMode(user, 0);
		tp->runAsThread(new OAuthFlow(np, sessions[user], user, sessions[user]->getTwitterUsername()));
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
		clearRoster(user);
		delete sessions[user];
		sessions[user] = NULL;
		connectionState[user] = DISCONNECTED;
		onlineUsers.erase(user);
	}
}

void TwitterPlugin::handleJoinRoomRequest(const std::string &user, const std::string &room, const std::string &nickname, const std::string &password)
{
	if(room == adminChatRoom) {
		
		LOG4CXX_INFO(logger, "Received Join Twitter room request for " << user)

		setTwitterMode(user, 2);
			
		handleParticipantChanged(user, adminNickName, room, 0, pbnetwork::STATUS_ONLINE);

		nickName[user] = nickname;

		handleMessage(user, adminChatRoom, "Connected to Twitter Room! Populating your followers list", adminNickName);
		
		tp->runAsThread(new FetchFriends(sessions[user], user,
										 boost::bind(&TwitterPlugin::populateRoster, this, _1, _2, _3)));
	} else {
		setTwitterMode(user, 0);
		LOG4CXX_ERROR(logger, "Couldn't connect to chatroom - " << room <<"! Try twitter-account as chatroom to access Twitter account")
		handleMessage(user, adminLegacyName, "Couldn't connect to chatroom! Try twitter-account as chatroom to access Twitter account");
	}	
}

void TwitterPlugin::handleLeaveRoomRequest(const std::string &user, const std::string &room)
{
	if(room == adminChatRoom && onlineUsers.count(user)) {
		LOG4CXX_INFO(logger, "Leaving chatroom! Switching back to default mode 0")
		setTwitterMode(user, 0);
		handleBuddyChanged(user, adminLegacyName, adminAlias, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
		//delete sessions[user];
		//sessions[user] = NULL;
		//connectionState[user] = DISCONNECTED;
		//onlineUsers.erase(user);
	}
}


void TwitterPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml) 
{

	LOG4CXX_INFO(logger, "Received " << user << " --> " << legacyName << " - " << message)
	
	if(legacyName == adminLegacyName || legacyName == adminChatRoom)  {

		//char ch;
		std::string cmd = "", data = "";
	 	
		int i;
		for(i=0 ; i<message.size() && message[i] != ' '; i++) cmd += message[i];
		while(i<message.size() && message[i] == ' ') i++;
		data = message.substr(i);
		
		//handleMessage(user, adminLegacyName, cmd + " " + data);

		if(cmd == "#pin") tp->runAsThread(new PINExchangeProcess(np, sessions[user], user, data));
		else if(cmd == "#help") tp->runAsThread(new HelpMessageRequest(user, boost::bind(&TwitterPlugin::helpMessageResponse, this, _1, _2)));
		else if(cmd[0] == '@') {
			std::string username = cmd.substr(1); 
			tp->runAsThread(new DirectMessageRequest(sessions[user], user, username, data,
												     boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
		}
		else if(cmd == "#status") tp->runAsThread(new StatusUpdateRequest(sessions[user], user, data,
														boost::bind(&TwitterPlugin::statusUpdateResponse, this, _1, _2)));
		else if(cmd == "#timeline") tp->runAsThread(new TimelineRequest(sessions[user], user, data, "",
														boost::bind(&TwitterPlugin::displayTweets, this, _1, _2, _3, _4)));
		else if(cmd == "#friends") tp->runAsThread(new FetchFriends(sessions[user], user,
													   boost::bind(&TwitterPlugin::displayFriendlist, this, _1, _2, _3)));
		else if(cmd == "#follow") tp->runAsThread(new CreateFriendRequest(sessions[user], user, data.substr(0,data.find('@')),
													   boost::bind(&TwitterPlugin::createFriendResponse, this, _1, _2, _3)));
		else if(cmd == "#unfollow") tp->runAsThread(new DestroyFriendRequest(sessions[user], user, data.substr(0,data.find('@')),
													   boost::bind(&TwitterPlugin::deleteFriendResponse, this, _1, _2, _3)));
		else if(cmd == "#retweet") tp->runAsThread(new RetweetRequest(sessions[user], user, data,
													   boost::bind(&TwitterPlugin::RetweetResponse, this, _1, _2)));
		else if(cmd == "#mode") {
			int m = 0;
			m = atoi(data.c_str());
			mode prevm = twitterMode[user];

			if((mode)m == twitterMode[user]) return;
			if(m < 0 || m > 2) {
				handleMessage(user, adminLegacyName, std::string("Error! Unknown mode ") + data + ". Allowed values 0,1,2." );
				return;
			}

			setTwitterMode(user, m);
			if((twitterMode[user] == SINGLECONTACT || twitterMode[user] == CHATROOM) && prevm == MULTIPLECONTACT) clearRoster(user);
			else if(twitterMode[user] == MULTIPLECONTACT) 
				tp->runAsThread(new FetchFriends(sessions[user], user, boost::bind(&TwitterPlugin::populateRoster, this, _1, _2, _3)));

			//if(twitterMode[user] != CHATROOM)
			//	handleBuddyChanged(user, adminLegacyName, adminAlias, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
			//
			handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
								std::string("Changed mode to ") + data, twitterMode[user] == CHATROOM ? adminNickName : "");
			LOG4CXX_INFO(logger, user << ": Changed mode to " << data  << " <" << (twitterMode[user] == CHATROOM ? adminNickName : "") << ">" )
		}

		else if(twitterMode[user] == CHATROOM) {
			std::string buddy = message.substr(0, message.find(":"));
			if(buddies[user].count(buddy) == 0) {
				tp->runAsThread(new StatusUpdateRequest(sessions[user], user, message,
														boost::bind(&TwitterPlugin::statusUpdateResponse, this, _1, _2)));
			} else {
				data = message.substr(message.find(":")+1);
				tp->runAsThread(new DirectMessageRequest(sessions[user], user, buddy, data,
												 		 boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
			}
		}
		else handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
				                 "Unknown command! Type #help for a list of available commands.", twitterMode[user] == CHATROOM ? adminNickName : "");
	} 

	else {	
		std::string buddy;
		if(twitterMode[user] == CHATROOM) buddy = legacyName.substr(legacyName.find("/") + 1);
		if(legacyName != "twitter") {
			tp->runAsThread(new DirectMessageRequest(sessions[user], user, buddy, message,
												 boost::bind(&TwitterPlugin::directMessageResponse, this, _1, _2, _3, _4)));
		}
	}
}

void TwitterPlugin::handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) 
{
	if(connectionState[user] != CONNECTED) {
		LOG4CXX_ERROR(logger, user << " is not connected to twitter!")
		return;
	}

	LOG4CXX_INFO(logger, user << " Adding Twitter contact " << buddyName)
	tp->runAsThread(new CreateFriendRequest(sessions[user], user, buddyName, 
											boost::bind(&TwitterPlugin::createFriendResponse, this, _1, _2, _3)));
}

void TwitterPlugin::handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) 
{
	if(connectionState[user] != CONNECTED) {
		LOG4CXX_ERROR(logger, user << " is not connected to twitter!")
		return;
	}
	tp->runAsThread(new DestroyFriendRequest(sessions[user], user, buddyName, 
											 boost::bind(&TwitterPlugin::deleteFriendResponse, this, _1, _2, _3)));
}


void TwitterPlugin::pollForTweets()
{
	boost::mutex::scoped_lock lock(userlock);
	std::set<std::string>::iterator it = onlineUsers.begin();
	while(it != onlineUsers.end()) {
		std::string user = *it;
		tp->runAsThread(new TimelineRequest(sessions[user], user, "", mostRecentTweetID[user],
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
		tp->runAsThread(new DirectMessageRequest(sessions[user], user, "", mostRecentDirectMessageID[user],
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

	twitterMode[user] = (mode)m;

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

void TwitterPlugin::OAuthFlowComplete(const std::string user, twitCurl *obj) 
{
	boost::mutex::scoped_lock lock(userlock);	

	delete sessions[user];
	sessions[user] = obj->clone();	
	connectionState[user] = WAITING_FOR_PIN;
}	

void TwitterPlugin::pinExchangeComplete(const std::string user, const std::string OAuthAccessTokenKey, const std::string OAuthAccessTokenSecret) 
{
	boost::mutex::scoped_lock lock(userlock);	
		
	sessions[user]->getOAuth().setOAuthTokenKey( OAuthAccessTokenKey );
	sessions[user]->getOAuth().setOAuthTokenSecret( OAuthAccessTokenSecret );
	connectionState[user] = CONNECTED;
	twitterMode[user] = (mode)getTwitterMode(user);
	
	if(twitterMode[user] == MULTIPLECONTACT) {
		tp->runAsThread(new FetchFriends(sessions[user], user, boost::bind(&TwitterPlugin::populateRoster, this, _1, _2, _3)));
	}

	onlineUsers.insert(user);
	mostRecentTweetID[user] = "";
	mostRecentDirectMessageID[user] = "";
}	

void TwitterPlugin::updateLastTweetID(const std::string user, const std::string ID)
{
	boost::mutex::scoped_lock lock(userlock);	
	mostRecentTweetID[user] = ID;
}

std::string TwitterPlugin::getMostRecentTweetID(const std::string user)
{
	boost::mutex::scoped_lock lock(userlock);	
	std::string ID = "-1";
	if(onlineUsers.count(user)) ID = mostRecentTweetID[user];
	return ID;
}

void TwitterPlugin::updateLastDMID(const std::string user, const std::string ID)
{
	boost::mutex::scoped_lock lock(userlock);	
	mostRecentDirectMessageID[user] = ID;
}

std::string TwitterPlugin::getMostRecentDMID(const std::string user)
{
	boost::mutex::scoped_lock lock(userlock);	
	std::string ID = "";
	if(onlineUsers.count(user)) ID = mostRecentDirectMessageID[user];
	return ID;
}

/************************************** Twitter response functions **********************************/
void TwitterPlugin::statusUpdateResponse(std::string &user, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");
	} else {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							"Status Update successful", twitterMode[user] == CHATROOM ? adminNickName : "");
	}
}

void TwitterPlugin::helpMessageResponse(std::string &user, std::string &msg)
{
	handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
						msg, twitterMode[user] == CHATROOM ? adminNickName : "");
}

void TwitterPlugin::clearRoster(const std::string user)
{
	if(buddies[user].size() == 0) return;
	std::set<std::string>::iterator it = buddies[user].begin();
	while(it != buddies[user].end()) {
		handleBuddyChanged(user, *it, *it, std::vector<std::string>(), pbnetwork::STATUS_NONE);
		handleBuddyRemoved(user, *it);
		it++;
	}
	buddies[user].clear();
}

void TwitterPlugin::populateRoster(std::string &user, std::vector<User> &friends, std::string &errMsg) 
{
	if(errMsg.length() == 0) 
	{
		for(int i=0 ; i<friends.size() ; i++) {
			if(twitterMode[user] == MULTIPLECONTACT)
				handleBuddyChanged(user, friends[i].getScreenName(), friends[i].getScreenName(), std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
			else if(twitterMode[user] == CHATROOM) {
				handleParticipantChanged(user, friends[i].getScreenName(), adminChatRoom, 0, pbnetwork::STATUS_ONLINE);
				buddies[user].insert(friends[i].getScreenName());
			}
		}
	} else handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							   std::string("Error populating roster - ") + errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");	

	if(twitterMode[user] == CHATROOM) handleParticipantChanged(user, nickName[user], adminChatRoom, 0, pbnetwork::STATUS_ONLINE);
}

void TwitterPlugin::displayFriendlist(std::string &user, std::vector<User> &friends, std::string &errMsg)
{
	if(errMsg.length() == 0) 
	{
		std::string userlist = "\n***************USER LIST****************\n";
		for(int i=0 ; i < friends.size() ; i++) {
			userlist += " - " + friends[i].getUserName() + " (" + friends[i].getScreenName() + ")\n";
		}	
		userlist += "***************************************\n";
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							userlist, twitterMode[user] == CHATROOM ? adminNickName : "");	
	} else handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName, 
							   errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");	
 
}

void TwitterPlugin::displayTweets(std::string &user, std::string &userRequested, std::vector<Status> &tweets , std::string &errMsg)
{
	if(errMsg.length() == 0) {
		std::string timeline = "";

		for(int i=0 ; i<tweets.size() ; i++) {

			if(twitterMode[user] != CHATROOM) {
				timeline += " - " + tweets[i].getUserData().getScreenName() + ": " + tweets[i].getTweet() + " (MsgId: " + tweets[i].getID() + ")\n";
			} else {
				handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
									tweets[i].getTweet() + " (MsgId: " + tweets[i].getID() + ")", tweets[i].getUserData().getScreenName());
			}
		}

		if((userRequested == "" || userRequested == user) && tweets.size()) {
			std::string tweetID = getMostRecentTweetID(user);
			if(tweetID != tweets[0].getID()) updateLastTweetID(user, tweets[0].getID());
		}

		if(timeline.length()) handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
												  timeline, twitterMode[user] == CHATROOM ? adminNickName : "");
	} else handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							   errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");	
}

void TwitterPlugin::directMessageResponse(std::string &user, std::string &username, std::vector<DirectMessage> &messages, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							std::string("Error while sending direct message! - ") + errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");	
		return;
	}

	if(username != "") {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
						   "Message delivered!", twitterMode[user] == CHATROOM ? adminNickName : "");	
		return;
	}
	
	if(!messages.size()) return;
	
	if(twitterMode[user] == SINGLECONTACT) {

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
				if(twitterMode[user] == MULTIPLECONTACT)
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

void TwitterPlugin::createFriendResponse(std::string &user, std::string &frnd, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");
		return;
	}

	handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
						std::string("You are now following ") + frnd, twitterMode[user] == CHATROOM ? adminNickName : "");
	
	//buddies[user].insert(frnd);
	//handleBuddyChanged(user, frnd, frnd, std::vector<std::string>(), pbnetwork::STATUS_NONE);

	if(twitterMode[user] == MULTIPLECONTACT) {
		handleBuddyChanged(user, frnd, frnd, std::vector<std::string>(), pbnetwork::STATUS_ONLINE);
	} else if(twitterMode[user] == CHATROOM) {
		buddies[user].insert(frnd);
		handleParticipantChanged(user, frnd, adminChatRoom, 0, pbnetwork::STATUS_ONLINE);
	}
}

void TwitterPlugin::deleteFriendResponse(std::string &user, std::string &frnd, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName, 
							errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");
		return;
	} 
	
	handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
						std::string("You are not following ") + frnd + " anymore", twitterMode[user] == CHATROOM ? adminNickName : "");
	
	if (twitterMode[user] == CHATROOM) {
		handleParticipantChanged(user, frnd, adminLegacyName, 0, pbnetwork::STATUS_NONE);
		buddies[user].erase(frnd);
	}
	//buddies[user].erase(frnd);
	//handleBuddyRemoved(user, frnd);
	if(twitterMode[user] == MULTIPLECONTACT) {
		handleBuddyRemoved(user, frnd);
	} 
}


void TwitterPlugin::RetweetResponse(std::string &user, std::string &errMsg)
{
	if(errMsg.length()) {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							errMsg, twitterMode[user] == CHATROOM ? adminNickName : "");
	} else {
		handleMessage(user, twitterMode[user] == CHATROOM ? adminChatRoom : adminLegacyName,
							"Retweet successful", twitterMode[user] == CHATROOM ? adminNickName : "");
	}
}
