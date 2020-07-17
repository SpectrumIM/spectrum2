#include "PINExchangeProcess.h"
DEFINE_LOGGER(pinExchangeProcessRequestLogger, "PINExchangeProcess")
void PINExchangeProcess::run()
{
	LOG4CXX_INFO(pinExchangeProcessRequestLogger, user << ": Sending PIN " << data);
	LOG4CXX_INFO(pinExchangeProcessRequestLogger, user << " " << twitObj->getProxyServerIp() << " " << twitObj->getProxyServerPort());
	twitObj->getOAuth().setOAuthPin( data );
	success = twitObj->oAuthAccessToken();
}

void PINExchangeProcess::finalize()
{
	if(!success) {
		LOG4CXX_ERROR(pinExchangeProcessRequestLogger, user << ": Error while exchanging PIN for Access Token!");
		np->handleMessage(user, "twitter.com", "Error while exchanging PIN for Access Token!");
		np->handleLogoutRequest(user, "");
	} else {
		std::string replyMsg;
		while(replyMsg.length() == 0) {
			twitObj->getLastWebResponse(replyMsg);
		}

		Error error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) {
			LOG4CXX_ERROR(pinExchangeProcessRequestLogger, user << ": Error while exchanging PIN for Access Token! " << error.getMessage());
			np->handleMessage(user, "twitter.com", error.getMessage());
			np->handleLogoutRequest(user, "");
			return;
		}


		std::string OAuthAccessTokenKey, OAuthAccessTokenSecret;
		twitObj->getOAuth().getOAuthTokenKey( OAuthAccessTokenKey );
		twitObj->getOAuth().getOAuthTokenSecret( OAuthAccessTokenSecret );

		if(np->storeUserOAuthKeyAndSecret(user, OAuthAccessTokenKey, OAuthAccessTokenSecret) == false) {
			np->handleLogoutRequest(user, "");
			return;
		}
		np->pinExchangeComplete(user, OAuthAccessTokenKey, OAuthAccessTokenSecret);
		np->handleMessage(user, "twitter.com", "PIN is OK. You are now authorized.");
		np->handleMessage(user, "twitter.com", "Send '#help' (without the quotes) to see how to use this transport.");
		LOG4CXX_INFO(pinExchangeProcessRequestLogger, user << ": Sent PIN " << data << " and obtained Access Token");
	}
}

/*void handlePINExchange(const std::string &user, std::string &data) {
	sessions[user]->getOAuth().setOAuthPin( data );
	if (sessions[user]->oAuthAccessToken() == false) {
		LOG4CXX_ERROR(pinExchangeProcessRequestLogger, user << ": Error while exchanging PIN for Access Token!");
		handleLogoutRequest(user, "");
		return;
	}

	std::string OAuthAccessTokenKey, OAuthAccessTokenSecret;
	sessions[user]->getOAuth().getOAuthTokenKey( OAuthAccessTokenKey );
	sessions[user]->getOAuth().getOAuthTokenSecret( OAuthAccessTokenSecret );

	UserInfo info;
	if(storagebackend->getUser(user, info) == false) {
		LOG4CXX_ERROR(pinExchangeProcessRequestLogger, "Didn't find entry for " << user << " in the database!");
		handleLogoutRequest(user, "");
		return;
	}

	storagebackend->updateUserSetting((long)info.id, OAUTH_KEY, OAuthAccessTokenKey);
	storagebackend->updateUserSetting((long)info.id, OAUTH_SECRET, OAuthAccessTokenSecret);

	connectionState[user] = CONNECTED;
	LOG4CXX_INFO(pinExchangeProcessRequestLogger, user << ": Sent PIN " << data << " and obtained Access Token");
}*/
