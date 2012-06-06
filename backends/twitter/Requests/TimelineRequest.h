#ifndef TIMELINE_H
#define TIMELINE_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterResponseParser.h"
#include "transport/networkplugin.h"
#include "../TwitterPlugin.h"
#include "transport/logging.h"
#include <string>
#include <iostream>

using namespace Transport;

class TimelineRequest : public Thread
{
	twitCurl *twitObj;
	std::string user;
	std::string userRequested;
	std::string replyMsg;
	std::string timeline;
	std::string since_id;
	TwitterPlugin *np;
	bool success;

	public:
	TimelineRequest(TwitterPlugin *_np, twitCurl *obj, const std::string &_user, const std::string &_user2, const std::string &_since_id) {
		twitObj = obj->clone();
		np = _np;
		user = _user;
		userRequested = _user2;
		since_id = _since_id;
	}

	~TimelineRequest() {
		//std::cerr << "*****Timeline request: DESTROYING twitObj****" << std::endl;
		delete twitObj;
	}

	void run();
	void finalize();
};
#endif
