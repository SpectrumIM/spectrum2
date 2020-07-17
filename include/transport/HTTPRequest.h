
#pragma once

#include "curl/curl.h"
#include "transport/Logging.h"
#include "transport/ThreadPool.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <json/json.h>

#include <Swiften/SwiftenCompat.h>

namespace Transport {

class HTTPRequest : public Thread {
	public:
		typedef enum { Get } Type;
		typedef boost::function< void (HTTPRequest *, bool, Json::Value &json, const std::string &data) > Callback;

		HTTPRequest(ThreadPool *tp, Type type, const std::string &url, Callback callback);
		HTTPRequest(Type type, const std::string &url);

		virtual ~HTTPRequest();

		void setProxy(std::string, std::string, std::string, std::string);
		bool execute();
		bool execute(Json::Value &json);
		std::string getError() {return std::string(curl_errorbuffer);}
		const std::string &getRawData() {
			return m_data;
		}

		void run();
		void finalize();

		const std::string &getURL() {
			return m_url;
		}

		SWIFTEN_SIGNAL_NAMESPACE::signal<void ()> onRequestFinished;

		static void globalInit() {
			curl_global_init(CURL_GLOBAL_ALL);
		}

		static void globalCleanup() {
			curl_global_cleanup();
		}

	private:
		bool init();
		bool GET(std::string url, std::string &output);
		bool GET(std::string url, Json::Value &json);


		CURL *curlhandle;
		char curl_errorbuffer[1024];
		std::string error;
		std::string callbackdata;
		ThreadPool *m_tp;
		std::string m_url;
		bool m_ok;
		Json::Value m_json;
		std::string m_data;
		Callback m_callback;
		Type m_type;

		static int curlCallBack(char* data, size_t size, size_t nmemb, HTTPRequest *obj);

};

}

