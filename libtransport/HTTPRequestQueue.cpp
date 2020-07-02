#include "transport/HTTPRequestQueue.h"
#include "transport/HTTPRequest.h"
#include "transport/Transport.h"

namespace Transport {

DEFINE_LOGGER(httpRequestQueueLogger, "HTTPRequestQueue")

HTTPRequestQueue::HTTPRequestQueue(Component *component, const std::string &user, int delay) {
	m_delay = delay;
	m_req = NULL;
	m_user = user;

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
		LOG4CXX_INFO(httpRequestQueueLogger, m_user << ": Queue is empty.");
		m_req = NULL;
		m_queueTimer->stop();
		return;
	}

	if (m_req) {
		LOG4CXX_INFO(httpRequestQueueLogger, m_user << ": There is already a request being handled.");
		return;
	}

	m_req = m_queue.front();
	m_queue.pop();

	LOG4CXX_INFO(httpRequestQueueLogger, m_user << ": Starting request '" << m_req->getURL() << "'.");
	m_req->onRequestFinished.connect(boost::bind(&HTTPRequestQueue::handleRequestFinished, this));
	m_req->execute();
}

void HTTPRequestQueue::queueRequest(HTTPRequest *req) {
	m_queue.push(req);

	if (!m_req) {
		sendNextRequest();
	}
	else {
		LOG4CXX_INFO(httpRequestQueueLogger, m_user << ": Request '" << req->getURL() << "' queued.");
	}
}


}
