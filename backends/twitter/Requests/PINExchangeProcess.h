#ifndef PIN_EXCHANGE
#define PIN_EXCHANGE

#include "transport/threadpool.h"
#include "../libtwitcurl/twitcurl.h"
#include "../TwitterPlugin.h"
#include "transport/networkplugin.h"
#include "transport/logging.h"

#include <string>
#include <iostream>

//class TwitterPlugin;
using namespace Transport;
class PINExchangeProcess : public Thread
{
	twitCurl *twitObj;
	std::string data;
	std::string user;
	TwitterPlugin *np;
	bool success;
	
	public:
	PINExchangeProcess(TwitterPlugin *_np, twitCurl *obj, const std::string &_user, const std::string &_data) {
		twitObj = obj->clone();
		data = _data;
		user = _user;
		np = _np;
	}

	~PINExchangeProcess() {
		delete twitObj;
	}	

	void run();
	void finalize();
};

#endif
