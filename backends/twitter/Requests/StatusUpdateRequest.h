#ifndef STATUS_UPDATE
#define STATUS_UPDATE

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <string>
#include <iostream>

using namespace Transport;
class StatusUpdateRequest : public Thread
{
	twitCurl *twitObj;
	std::string data;
	std::string user;
	std::string replyMsg;
	NetworkPlugin *np;
	public:
	StatusUpdateRequest(NetworkPlugin *_np, twitCurl *obj, const std::string &_user, const std::string &_data) {
		twitObj = obj->clone();
		data = _data;
		user = _user;
		np = _np;
	}

	~StatusUpdateRequest() {
		delete twitObj;
	}

	void run();
	void finalize();
};

#endif
