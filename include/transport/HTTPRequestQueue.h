
#pragma once

#include "curl/curl.h"
#include "transport/Logging.h"
#include "transport/ThreadPool.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include "rapidjson/document.h"

#include "Swiften/Network/Timer.h"

namespace Transport {

class HTTPRequest;
class Component;

class HTTPRequestQueue {
	public:
		HTTPRequestQueue(Component *component, int delayBetweenRequests = 1);

		virtual ~HTTPRequestQueue();

		void queueRequest(HTTPRequest *req);

		void sendNextRequest();

	private:
		void handleRequestFinished();

	private:
		int m_delay;
		std::queue<HTTPRequest *> m_queue;
		HTTPRequest *m_req;
		Swift::Timer::ref m_queueTimer;
};

}

