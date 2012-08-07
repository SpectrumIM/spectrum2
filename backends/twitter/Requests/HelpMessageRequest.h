#ifndef HELPMESSAGE_H
#define HELPMESSAGE_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <string>
#include <boost/function.hpp>
#include <iostream>

using namespace Transport;

class HelpMessageRequest : public Thread
{
	std::string user;
	std::string helpMsg;
	boost::function<void (std::string &, std::string &)> callBack;
	
	public:
	HelpMessageRequest(const std::string &_user, boost::function<void (std::string &, std::string &)> cb) {
		user = _user;
		callBack = cb;
	}

	void run();
	void finalize();
};

#endif
