#ifndef FRIENDS_H
#define FRIENDS_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterResponseParser.h"
#include "transport/logging.h"
#include <string>
#include <boost/signals.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace Transport;

class FetchFriends : public Thread
{
	twitCurl *twitObj;
	std::string user;
	std::string replyMsg;
	std::vector<User> friends;
	std::vector<std::string> friendAvatars;
	bool success;
	boost::function< void (std::string, std::vector<User> &, std::vector<std::string> &, std::string) > callBack;

	public:
	FetchFriends(twitCurl *obj, const std::string &_user, 
			     boost::function< void (std::string, std::vector<User> &, std::vector<std::string> &, std::string) >  cb) {
		twitObj = obj->clone();
		user = _user;
		callBack = cb;
	}

	~FetchFriends() {
		delete twitObj;
	}

	void run();
	void finalize();
};
#endif
