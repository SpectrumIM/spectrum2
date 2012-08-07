#ifndef TIMELINE_H
#define TIMELINE_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterResponseParser.h"
#include "transport/logging.h"
#include <string>
#include <iostream>
#include <boost/function.hpp>

using namespace Transport;

class TimelineRequest : public Thread
{
	twitCurl *twitObj;
	std::string user;
	std::string userRequested;
	std::string replyMsg;
	std::string since_id;
	bool success;
	boost::function< void (std::string&, std::string&, std::vector<Status> &, std::string&) > callBack;
	std::vector<Status> tweets;

	public:
	TimelineRequest(twitCurl *obj, const std::string &_user, const std::string &_user2, const std::string &_since_id,
					boost::function< void (std::string&, std::string&, std::vector<Status> &, std::string&) > cb) {
		twitObj = obj->clone();
		user = _user;
		userRequested = _user2;
		since_id = _since_id;
		callBack = cb;
	}

	~TimelineRequest() {
		//std::cerr << "*****Timeline request: DESTROYING twitObj****" << std::endl;
		delete twitObj;
	}

	void run();
	void finalize();
};
#endif
