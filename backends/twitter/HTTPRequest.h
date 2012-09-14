#ifndef HTTPREQ_H
#define HTTPREQ_H

#include "libtwitcurl/curl/curl.h"
#include "transport/logging.h"
#include <iostream>
#include <sstream>
#include <string.h>

class HTTPRequest
{
	CURL *curlhandle;
	char curl_errorbuffer[1024];
	std::string error;
	std::string callbackdata;

    static int curlCallBack(char* data, size_t size, size_t nmemb, HTTPRequest *obj);

	public:
	HTTPRequest() {
		curlhandle = NULL;
	}

	~HTTPRequest() {
		if(curlhandle) {
			curl_easy_cleanup(curlhandle);
			curlhandle = NULL;
		}
	}

	bool init();
	void setProxy(std::string, std::string, std::string, std::string);
	bool GET(std::string, std::string &);
	std::string getCurlError() {return std::string(curl_errorbuffer);}
};

#endif
