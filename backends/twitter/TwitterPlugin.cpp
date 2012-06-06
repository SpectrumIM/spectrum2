#include "TwitterPlugin.h"
#include "Requests/StatusUpdateRequest.h"
#include "Requests/DirectMessageRequest.h"
#include "Requests/TimelineRequest.h"
#include "Requests/FetchFriends.h"
#include "Requests/HelpMessageRequest.h"
#include "Requests/PINExchangeProcess.h"
#include "Requests/OAuthFlow.h"

DEFINE_LOGGER(logger, "Twitter Backend");

TwitterPlugin *np = NULL;
Swift::SimpleEventLoop *loop_; // Event Loop
TwitterPlugin::TwitterPlugin(Config *config, Swift::SimpleEventLoop *loop, StorageBackend *storagebackend, const std::string &host, int port) : NetworkPlugin() 
{
	this->config = config;
	this->storagebackend = storagebackend;

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
		
	m_timer = m_factories->getTimerFactory()->createTimer(60000);
	m_timer->onTick.connect(boost::bind(&TwitterPlugin::pollForTweets, this));
	m_timer->start();
	
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
void TwitterPlugin::handleLogoutRequest(const std::string &user, const std::string &legacyName) 
{
	delete sessions[user];
	sessions[user] = NULL;
	connectionState[user] = DISCONNECTED;
	onlineUsers.erase(user);
}


void TwitterPlugin::handleMessageSendRequest(const std::string &user, const std::string &legacyName, const std::string &message, const std::string &xhtml) 
{
	
	if(legacyName == "twitter-account") {

		std::string cmd = "", data = "";
		std::istringstream in(message.c_str());
		in >> cmd >> data;
		
		//handleMessage(user, "twitter-account", cmd + " " + data);

		if(cmd == "#pin") tp->runAsThread(new PINExchangeProcess(np, sessions[user], user, data));
		else if(cmd == "#help") tp->runAsThread(new HelpMessageRequest(np, user));
		else if(cmd[0] == '@') {
			std::string username = cmd.substr(1); 
			tp->runAsThread(new DirectMessageRequest(np, sessions[user], user, username, data));
		}
		else if(cmd == "#status") tp->runAsThread(new StatusUpdateRequest(np, sessions[user], user, data));
		else if(cmd == "#timeline") tp->runAsThread(new TimelineRequest(np, sessions[user], user, data));
		else if(cmd == "#friends") tp->runAsThread(new FetchFriends(np, sessions[user], user));
		else handleMessage(user, "twitter-account", "Unknown command! Type #help for a list of available commands.");
	}
}

void TwitterPlugin::handleBuddyUpdatedRequest(const std::string &user, const std::string &buddyName, const std::string &alias, const std::vector<std::string> &groups) 
{
	LOG4CXX_INFO(logger, user << ": Added buddy " << buddyName << ".");
	handleBuddyChanged(user, buddyName, alias, groups, pbnetwork::STATUS_ONLINE);
}

void TwitterPlugin::handleBuddyRemovedRequest(const std::string &user, const std::string &buddyName, const std::vector<std::string> &groups) 
{

}


void TwitterPlugin::pollForTweets()
{
	boost::mutex::scoped_lock lock(userlock);
	std::set<std::string>::iterator it = onlineUsers.begin();
	while(it != onlineUsers.end()) {
		std::string user = *it;
		tp->runAsThread(new TimelineRequest(np, sessions[user], user, ""));
		it++;
	}
	m_timer->start();
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
	onlineUsers.insert(user);
}	
