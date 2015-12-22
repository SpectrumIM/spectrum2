#include "transport/HTTPRequestQueue.h"
#include "transport/HTTPRequest.h"
#include "transport/Transport.h"

namespace Transport {

DEFINE_LOGGER(logger, "HTTPRequestQueue")

HTTPRequestQueue::HTTPRequestQueue(Component *component, int delay) {
	m_delay = delay;
	m_req = NULL;

	m_queueTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(500);
	m_queueTimer->onTick.connect(boost::bind(&HTTPRequestQueue::sendNextRequest, this));
}

HTTPRequestQueue::~HTTPRequestQueue() {
	m_queueTimer->stop();

	if (m_req) {
		m_req->onRequestFinished.disconnect(boost::bind(&HTTPRequestQueue::handleRequestFinished, this));
	}
}

void HTTPRequestQueue::handleRequestFinished() {
	m_req = NULL;
	m_queueTimer->start();
}

void HTTPRequestQueue::sendNextRequest() {
	if (m_queue.empty()) {
		LOG4CXX_INFO(logger, "queue is empty");
		m_req = NULL;
		m_queueTimer->stop();
		return;
	}

	if (m_req) {
		LOG4CXX_INFO(logger, "we are handling the request");
		return;
	}

	LOG4CXX_INFO(logger, "Starting new request");
	m_req = m_queue.front();
	m_queue.pop();
	m_req->onRequestFinished.connect(boost::bind(&HTTPRequestQueue::handleRequestFinished, this));
	m_req->execute();
}

void HTTPRequestQueue::queueRequest(HTTPRequest *req) {
	m_queue.push(req);

	LOG4CXX_INFO(logger, "request queued");
	if (!m_req) {
		sendNextRequest();
	}
}


}
