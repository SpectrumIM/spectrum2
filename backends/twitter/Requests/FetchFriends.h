#ifndef FRIENDS_H
#define FRIENDS_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterResponseParser.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <string>
#include <iostream>

using namespace Transport;

class FetchFriends : public Thread
{
	twitCurl *twitObj;
	std::string user;
	std::string replyMsg;
	std::string userlist;
	NetworkPlugin *np;

	public:
	FetchFriends(NetworkPlugin *_np, twitCurl *obj, const std::string &_user) {
		twitObj = obj->clone();
		np = _np;
		user = _user;
	}

	~FetchFriends() {
		delete twitObj;
	}

	void run();
	void finalize();
};
#endif
