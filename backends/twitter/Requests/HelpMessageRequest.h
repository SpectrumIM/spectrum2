#ifndef HELPMESSAGE_H
#define HELPMESSAGE_H

#include "../ThreadPool.h"
#include "../libtwitcurl/twitcurl.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"
#include <string>
#include <iostream>

using namespace Transport;

class HelpMessageRequest : public Thread
{
	std::string user;
	NetworkPlugin *np;

	public:
	HelpMessageRequest(NetworkPlugin *_np, const std::string &_user) {
		user = _user;
		np = _np;
	}

	void run();
	void finalize();
};

#endif
