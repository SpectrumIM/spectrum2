#ifndef DIRECT_MESSAGE
#define DIRECT_MESSAGE

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <string>
#include <iostream>

using namespace Transport;

class DirectMessageRequest : public Thread
{
	twitCurl *twitObj;
	std::string data;
	std::string user;
	std::string username;
	std::string replyMsg;
	NetworkPlugin *np;

	public:
	DirectMessageRequest(NetworkPlugin *_np, twitCurl *obj, const std::string &_user, const std::string & _username, const std::string &_data) {
		twitObj = obj->clone();
		data = _data;
		user = _user;
		username = _username;
		np = _np;
	}

	~DirectMessageRequest() {
		delete twitObj;
	}

	void run();
	void finalize();
};

#endif
