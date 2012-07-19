#ifndef PROFILEIMAGE_H
#define PROFILEIMAGE_H

#include "../ThreadPool.h"
#include "../libtwitcurl/curl/curl.h"
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


	bool success;
	CURL *curlhandle;
	
	std::string error;
	std::string callbackdata;
	char curl_errorbuffer[1024];
	bool fetchImage();
    static int curlCallback( char* data, size_t size, size_t nmemb, ProfileImageRequest *obj);

	public:
	ProfileImageRequest(Config *config, const std::string &_user, const std::string &_buddy, const std::string &_url, unsigned int _reqID,
			     boost::function< void (std::string&, std::string&, std::string&, int, std::string&) >  cb) {
		
		curlhandle = curl_easy_init();
		curl_easy_setopt(curlhandle, CURLOPT_PROXY, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_PROXYUSERPWD, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_PROXYAUTH, (long)CURLAUTH_ANY);

		/************ Set proxy if available *****************************************/
		if(CONFIG_HAS_KEY(config,"proxy.server")) {
			std::string ip = CONFIG_STRING(config,"proxy.server");

			std::ostringstream out; 
			out << CONFIG_INT(config,"proxy.port");
			std::string port = out.str();

			std::string puser = CONFIG_STRING(config,"proxy.user");
			std::string ppasswd = CONFIG_STRING(config,"proxy.password");
	
			if(ip != "localhost" && port != "0") {
				/* Set proxy details in cURL */
				std::string proxyIpPort = ip + ":" + port;
				curl_easy_setopt(curlhandle, CURLOPT_PROXY, proxyIpPort.c_str());

				/* Prepare username and password for proxy server */
				if(puser.length() && ppasswd.length()) {
					std::string proxyUserPass = puser + ":" + ppasswd;
					curl_easy_setopt(curlhandle, CURLOPT_PROXYUSERPWD, proxyUserPass.c_str());
				}
			}
		}
		/*****************************************************************************/

		user = _user;
		buddy = _buddy;
		url = _url;
		reqID = _reqID;
		callBack = cb;
	}

	~ProfileImageRequest() {
		if(curlhandle) {
			curl_easy_cleanup(curlhandle);
			curlhandle = NULL;
		}
	}

	void run();
	void finalize();
};
#endif
