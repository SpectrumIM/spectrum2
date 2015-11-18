#ifndef STATUS_UPDATE
#define STATUS_UPDATE

#include "transport/ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterResponseParser.h"
#include "transport/Logging.h"
#include <boost/function.hpp>
#include <string>
#include <iostream>

using namespace Transport;
class StatusUpdateRequest : public Thread
{
	twitCurl *twitObj;
	std::string data;
	std::string user;
	std::string replyMsg;
	boost::function<void (std::string& user, Error& errMsg)> callBack;
	bool success;

	public:
	StatusUpdateRequest(twitCurl *obj, const std::string &_user, const std::string &_data,
						boost::function<void (std::string& user, Error& errMsg)> cb) {
		twitObj = obj->clone();
		data = _data;
		user = _user;
		callBack = cb;
	}

	~StatusUpdateRequest() {
		delete twitObj;
	}

	void run();
	void finalize();
};

#endif
