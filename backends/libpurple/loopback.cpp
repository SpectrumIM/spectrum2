#include "loopback.h"
#include "transport/Logging.h"
#include "purple_defs.h"

DEFINE_LOGGER(logger_loopback, "backend/MessageLoopbackTracker");


MessageLoopbackTracker::MessageLoopbackTracker()
	: m_timer(0)
{
	LOG4CXX_TRACE(logger_loopback, "MessageLoopbackTracker()");
}

MessageLoopbackTracker::~MessageLoopbackTracker() {
	LOG4CXX_TRACE(logger_loopback, "~MessageLoopbackTracker()");
	this->setAutotrim(false);
}

void MessageLoopbackTracker::add(PurpleConversation *conv, const std::string &message) {
	MessageFingerprint fp;
	fp.conv = conv;
	fp.when = time(0);
	fp.message = message;
	LOG4CXX_TRACE(logger_loopback, "add("<<conv<<","<<fp.when<<","<<message<<")");
	this->push_back(fp);
};

bool MessageLoopbackTracker::matchAndRemove(PurpleConversation *conv, const std::string &message, const time_t &when)
{
	for (iterator it=this->begin(); it!=this->end(); it++)
		if ((it->conv==conv)
		&& (when-(it->when) >= 0) && (when-(it->when) < CarbonTimeout)
		&& (it->message==message)) {
			this->erase(it);
			LOG4CXX_TRACE(logger_loopback, "match("<<conv<<","<<when<<","<<message<<") -> true");
			return true;
		}
	LOG4CXX_TRACE(logger_loopback, "match("<<conv<<","<<when<<","<<message<<") -> false");
	return false;
}

void MessageLoopbackTracker::trim(const time_t min_time) {
	for (iterator it=this->begin(); it!=this->end();) {
		if (it->when > min_time)
			break;
		LOG4CXX_TRACE(logger_loopback, "trim("<<it->conv<<","<<it->when<<","<<it->message<<")");
		it = this->erase(it);
	}
}

static gboolean messageloopback_maintenance(void *data) {
	reinterpret_cast<MessageLoopbackTracker*>(data)->trim(time(0)-MessageLoopbackTracker::CarbonTimeout);
}

void MessageLoopbackTracker::setAutotrim(bool enableAutotrim) {
	if (enableAutotrim == (this->m_timer != 0))
		return;
	if (enableAutotrim)
		this->m_timer = purple_timeout_add_wrapped(1000, messageloopback_maintenance, this);
	else {
		purple_timeout_remove_wrapped(this->m_timer);
		this->m_timer == 0;
	}
}
