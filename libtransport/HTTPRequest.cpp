#include "transport/HTTPRequest.h"

namespace Transport {

DEFINE_LOGGER(logger, "HTTPRequest")

HTTPRequest::HTTPRequest(ThreadPool *tp, Type type, const std::string &url, Callback callback) {
	m_type = type;
	m_url = url;
	m_tp = tp;
	m_callback = callback;
	curlhandle = NULL;
}

HTTPRequest::HTTPRequest(Type type, const std::string &url) {
	m_type = type;
	m_url = url;
	m_tp = NULL;
	curlhandle = NULL;
}

HTTPRequest::~HTTPRequest() {
	if (curlhandle) {
		LOG4CXX_INFO(logger, "Cleaning up CURL handle");
		curl_easy_cleanup(curlhandle);
		curlhandle = NULL;
	}
}

bool HTTPRequest::init() {
	if (curlhandle) {
		return true;
	}

	curlhandle = curl_easy_init();
	if (curlhandle) {
		curl_easy_setopt(curlhandle, CURLOPT_PROXY, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_PROXYUSERPWD, NULL);
		curl_easy_setopt(curlhandle, CURLOPT_PROXYAUTH, (long)CURLAUTH_ANY);
		return true;
	}

	LOG4CXX_ERROR(logger, "Couldn't Initialize curl!");
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
		LOG4CXX_ERROR(logger, "Trying to set proxy while CURL isn't initialized");
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
		curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(curlhandle, CURLOPT_URL, url.c_str());

		/* Send http request and return status*/
		if(CURLE_OK == curl_easy_perform(curlhandle)) {
			data = callbackdata;
			return true;
		}
	} else {
		LOG4CXX_ERROR(logger, "CURL not initialized!");
		strcpy(curl_errorbuffer, "CURL not initialized!");
	}
	LOG4CXX_ERROR(logger, "Error fetching " << url);
	return false;
}

bool HTTPRequest::GET(std::string url, rapidjson::Document &json) {
	if (!GET(url, m_data)) {
		return false;
	}

	if(json.Parse<0>(m_data.c_str()).HasParseError()) {
		LOG4CXX_ERROR(logger, "Error while parsing JSON");
        LOG4CXX_ERROR(logger, m_data);
		strcpy(curl_errorbuffer, "Error while parsing JSON");
		return false;
	}

	return true;
}

void HTTPRequest::run() {
	if (!init()) {
		m_ok =  false;
		return;
	}

	switch (m_type) {
		case Get:
			m_ok = GET(m_url, m_json);
			break;
	}

	curl_easy_cleanup(curlhandle);
	curlhandle = NULL;
}

void HTTPRequest::finalize() {
	m_callback(this, m_ok, m_json, m_data);
	onRequestFinished();
}

bool HTTPRequest::execute() {
	if (!m_tp) {
		return false;
	}

	m_tp->runAsThread(this);
	return true;
}

bool HTTPRequest::execute(rapidjson::Document &json) {
	init();
	switch (m_type) {
		case Get:
			m_ok = GET(m_url, json);
			break;
	}

	return m_ok;
}

}
