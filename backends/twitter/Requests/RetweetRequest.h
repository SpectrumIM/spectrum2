#ifndef RETWEET_H
#define RETWEET_H

#include "../ThreadPool.h"
#include "../TwitterResponseParser.h"
#include "../libtwitcurl/twitcurl.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <boost/function.hpp>
#include <string>
#include <iostream>

using namespace Transport;
class RetweetRequest : public Thread
{
	twitCurl *twitObj;
	std::string data;
	std::string user;
	std::string replyMsg;
	bool success;
	boost::function < void (std::string&, std::string &) > callBack;

	public:
	RetweetRequest(twitCurl *obj, const std::string &_user, const std::string &_data,
			       boost::function < void (std::string &, std::string &) > _cb) {
		twitObj = obj->clone();
		data = _data;
		user = _user;
		callBack = _cb;
	}

	~RetweetRequest() {
		delete twitObj;
	}

	void run();
	void finalize();
};

#endif
