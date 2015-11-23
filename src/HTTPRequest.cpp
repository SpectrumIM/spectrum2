#include "transport/HTTPRequest.h"

namespace Transport {

DEFINE_LOGGER(logger, "HTTPRequest")

HTTPRequest::HTTPRequest() : curlhandle(NULL) {
	init();
}

bool HTTPRequest::init() {
	curlhandle = curl_easy_init();
	if (curlhandle) {
		curlhandle = curl_easy_init();
		curl_easy_setopt(curlhandle, CURLOPT_PROXY, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_PROXYUSERPWD, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_PROXYAUTH, (long)CURLAUTH_ANY);
		return true;
	}

	LOG4CXX_ERROR(logger, "Couldn't Initialize curl!")
	return false;
}

void HTTPRequest::setProxy(std::string IP, std::string port, std::string username, std::string password) {
	if (curlhandle) {
		std::string proxyIpPort = IP + ":" + port;
		curl_easy_setopt(curlhandle, CURLOPT_PROXY, proxyIpPort.c_str());
		if(username.length() && password.length()) {
			std::string proxyUserPass = username + ":" + password;
			curl_easy_setopt(curlhandle, CURLOPT_PROXYUSERPWD, proxyUserPass.c_str());
		}
	} else {
		LOG4CXX_ERROR(logger, "Trying to set proxy while CURL isn't initialized")
	}
}

int HTTPRequest::curlCallBack(char* data, size_t size, size_t nmemb, HTTPRequest* obj) {
	int writtenSize = 0;
	if (obj && data) {
		obj->callbackdata.append(data, size*nmemb);
		writtenSize = (int)(size*nmemb);
	}

	return writtenSize;
}

bool HTTPRequest::GET(std::string url, 	std::string &data) {
	if (curlhandle) {
		curl_easy_setopt(curlhandle, CURLOPT_CUSTOMREQUEST, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_ENCODING, "");
		
		data = "";
		callbackdata = "";
		memset(curl_errorbuffer, 0, 1024);
		
		curl_easy_setopt(curlhandle, CURLOPT_ERRORBUFFER, curl_errorbuffer);
		curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, curlCallBack);
		curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, this);
			
		/* Set http request and url */
		curl_easy_setopt(curlhandle, CURLOPT_HTTPGET, 1);
		curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());
		
		/* Send http request and return status*/
		if(CURLE_OK == curl_easy_perform(curlhandle)) {
			data = callbackdata;
			return true;
		}
	} else {
		LOG4CXX_ERROR(logger, "CURL not initialized!")
		strcpy(curl_errorbuffer, "CURL not initialized!");
	}
	return false;
}

bool HTTPRequest::GET(std::string url, rapidjson::Document &json) {
	std::string data;
	if (!GET(url, data)) {
		return false;
	}

	if(json.Parse<0>(data.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON")
        LOG4CXX_ERROR(logger, data)
		return false;
	}

	return true;
}

}
