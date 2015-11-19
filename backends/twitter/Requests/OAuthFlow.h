#ifndef OAUTH_FLOW
#define OAUTH_FLOW

#include "transport/ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterPlugin.h"
#include "transport/Logging.h"

#include <string>
#include <iostream>

//class TwitterPlugin;
using namespace Transport;
class OAuthFlow : public Thread
{
	twitCurl *twitObj;
	std::string username;
	std::string user;
	std::string authUrl;
	TwitterPlugin *np;
	bool success;
	
	public:
	OAuthFlow(TwitterPlugin *_np, twitCurl *obj, const std::string &_user, const std::string &_username) {
		twitObj = obj->clone();
		username = _username;
		user = _user;
		np = _np;
	}

	~OAuthFlow() {
		delete twitObj;
	}	

	void run();
	void finalize();
};

#endif
