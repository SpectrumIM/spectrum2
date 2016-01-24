
#pragma once

#include "curl/curl.h"
#include "transport/Logging.h"
#include "transport/ThreadPool.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include "rapidjson/document.h"

#include <boost/signal.hpp>

namespace Transport {

class HTTPRequest : public Thread {
	public:
		typedef enum { Get } Type;
		typedef boost::function< void (HTTPRequest *, bool, rapidjson::Document &json, const std::string &data) > Callback;

		HTTPRequest(ThreadPool *tp, Type type, const std::string &url, Callback callback);
		HTTPRequest(Type type, const std::string &url);

		virtual ~HTTPRequest() {
			if(curlhandle) {
				curl_easy_cleanup(curlhandle);
				curlhandle = NULL;
			}
		}

		void setProxy(std::string, std::string, std::string, std::string);
		bool execute();
		bool execute(rapidjson::Document &json);
		std::string getError() {return std::string(curl_errorbuffer);}
		const std::string &getRawData() {
			return m_data;
		}

		void run();
		void finalize();

		boost::signal<void ()> onRequestFinished;

	private:
		bool init();
		bool GET(std::string url, std::string &output);
		bool GET(std::string url, rapidjson::Document &json);


		CURL *curlhandle;
		char curl_errorbuffer[1024];
		std::string error;
		std::string callbackdata;
		ThreadPool *m_tp;
		std::string m_url;
		bool m_ok;
		rapidjson::Document m_json;
		std::string m_data;
		Callback m_callback;
		Type m_type;

		static int curlCallBack(char* data, size_t size, size_t nmemb, HTTPRequest *obj);

};

}

