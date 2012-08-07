#ifndef DESTROY_FRIEND
#define DESTROY_FRIEND

#include "transport/threadpool.h"
#include "../TwitterResponseParser.h"
#include "../libtwitcurl/twitcurl.h"
#include "transport/logging.h"
#include <string>
#include <boost/function.hpp>
#include <iostream>
#include <vector>

using namespace Transport;

class DestroyFriendRequest : public Thread
{
	twitCurl *twitObj;
	std::string user;
	std::string frnd;
	std::string replyMsg;
	boost::function< void (std::string&, User&, std::string&) > callBack;
	User friendInfo;	
	bool success;

	public:
	DestroyFriendRequest(twitCurl *obj, const std::string &_user, const std::string & _frnd,
			     		 boost::function< void (std::string&, User&, std::string&) >  cb) {
		twitObj = obj->clone();
		user = _user;
		frnd = _frnd;
		callBack = cb;
	}

	~DestroyFriendRequest() {
		delete twitObj;
	}

	void run();
	void finalize();
};

#endif
