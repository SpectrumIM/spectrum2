#include "transport/HTTPRequestQueue.h"
#include "transport/HTTPRequest.h"

namespace Transport {

DEFINE_LOGGER(logger, "HTTPRequestQueue")

HTTPRequestQueue::HTTPRequestQueue(int delay) {
	m_delay = delay;
	m_processing = false;
}

HTTPRequestQueue::~HTTPRequestQueue() {
	
}

void HTTPRequestQueue::sendNextRequest() {
	if (m_queue.empty()) {
		m_processing = false;
		return;
	}

	if (m_processing) {
		return;
	}

	HTTPRequest *req = m_queue.front();
	m_queue.pop();
	req->onRequestFinished.connect(boost::bind(&HTTPRequestQueue::sendNextRequest, this));
	req->execute();
}

void HTTPRequestQueue::queueRequest(HTTPRequest *req) {
	m_queue.push(req);

	if (!m_processing) {
		sendNextRequest();
	}
}


}
