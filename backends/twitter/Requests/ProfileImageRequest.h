#ifndef PROFILEIMAGE_H
#define PROFILEIMAGE_H

#include "../ThreadPool.h"
#include "../TwitterResponseParser.h"
#include "transport/logging.h"
#include "transport/config.h"
#include <string>
#include <boost/signals.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <sstream>

using namespace Transport;
using namespace boost::program_options;

class ProfileImageRequest : public Thread
{
	std::string user;
	std::string buddy;
	std::string url;
	std::string img;
	unsigned int reqID;
	boost::function< void (std::string&, std::string&, std::string&, int, std::string&) > callBack;

	std::string ip, port, puser, ppasswd;

	bool success;

	std::string error;
	std::string callbackdata;

	public:
	ProfileImageRequest(Config *config, const std::string &_user, const std::string &_buddy, const std::string &_url, unsigned int _reqID,
			     boost::function< void (std::string&, std::string&, std::string&, int, std::string&) >  cb) {

		if(CONFIG_HAS_KEY(config,"proxy.server")) {
			ip = CONFIG_STRING(config,"proxy.server");

			std::ostringstream out; 
			out << CONFIG_INT(config,"proxy.port");
			port = out.str();

			puser = CONFIG_STRING(config,"proxy.user");
			ppasswd = CONFIG_STRING(config,"proxy.password");
		}

		user = _user;
		buddy = _buddy;
		url = _url;
		reqID = _reqID;
		callBack = cb;
	}

	~ProfileImageRequest() {
	}

	void run();
	void finalize();
};
#endif
