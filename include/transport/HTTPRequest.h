#ifndef HTTPREQ_H
#define HTTPREQ_H

#include "curl/curl.h"
#include "transport/Logging.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include "rapidjson/document.h"

namespace Transport {

class HTTPRequest {
	public:
		HTTPRequest();

		~HTTPRequest() {
			if(curlhandle) {
				curl_easy_cleanup(curlhandle);
				curlhandle = NULL;
			}
		}

		void setProxy(std::string, std::string, std::string, std::string);
		bool GET(std::string url, std::string &output);
		bool GET(std::string url, rapidjson::Document &json);
		std::string getCurlError() {return std::string(curl_errorbuffer);}

	private:
		bool init();

		CURL *curlhandle;
		char curl_errorbuffer[1024];
		std::string error;
		std::string callbackdata;

		static int curlCallBack(char* data, size_t size, size_t nmemb, HTTPRequest *obj);

};

}

#endif
