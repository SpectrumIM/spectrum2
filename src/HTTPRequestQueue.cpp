#include "transport/HTTPRequestQueue.h"
#include "transport/HTTPRequest.h"
#include "transport/Transport.h"

namespace Transport {

DEFINE_LOGGER(logger, "HTTPRequestQueue")

HTTPRequestQueue::HTTPRequestQueue(Component *component, int delay) {
	m_delay = delay;
	m_processing = false;

	m_queueTimer = component->getNetworkFactories()->getTimerFactory()->createTimer(500);
	m_queueTimer->onTick.connect(boost::bind(&HTTPRequestQueue::sendNextRequest, this));
}

HTTPRequestQueue::~HTTPRequestQueue() {
	m_queueTimer->stop();
}

void HTTPRequestQueue::handleRequestFinished() {
	m_queueTimer->start();
}

void HTTPRequestQueue::sendNextRequest() {
	if (m_queue.empty()) {
		m_processing = false;
		m_queueTimer->stop();
		return;
	}

	if (m_processing) {
		return;
	}

	HTTPRequest *req = m_queue.front();
	m_queue.pop();
	req->onRequestFinished.connect(boost::bind(&HTTPRequestQueue::handleRequestFinished, this));
	req->execute();
}

void HTTPRequestQueue::queueRequest(HTTPRequest *req) {
	m_queue.push(req);

	if (!m_processing) {
		sendNextRequest();
	}
}


}
