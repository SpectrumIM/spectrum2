#ifndef TIMELINE_H
#define TIMELINE_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterResponseParser.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <string>
#include <iostream>

using namespace Transport;

class TimelineRequest : public Thread
{
	twitCurl twitObj;
	std::string user;
	std::string replyMsg;
	std::string timeline;
	NetworkPlugin *np;

	public:
	TimelineRequest(NetworkPlugin *_np, twitCurl *obj, const std::string &_user) {
		twitObj = *obj;
		np = _np;
		user = _user;
	}

	void run();
	void finalize();
};
#endif
