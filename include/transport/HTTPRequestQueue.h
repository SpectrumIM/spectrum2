
#pragma once

#include "curl/curl.h"
#include "transport/Logging.h"
#include "transport/ThreadPool.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include "rapidjson/document.h"

namespace Transport {

class HTTPRequest;

class HTTPRequestQueue {
	public:
		HTTPRequestQueue(int delayBetweenRequests = 1);

		virtual ~HTTPRequestQueue();

		void queueRequest(HTTPRequest *req);

		void sendNextRequest();

	private:
		int m_delay;
		std::queue<HTTPRequest *> m_queue;
		bool m_processing;
};

}

