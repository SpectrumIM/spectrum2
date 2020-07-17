#include "StatusUpdateRequest.h"
#include "../TwitterResponseParser.h"

DEFINE_LOGGER(statusUpdateRequestLogger, "StatusUpdateRequest")
void StatusUpdateRequest::run()
{
	replyMsg = "";
	success = twitObj->statusUpdate(data);
	if(success) {
		twitObj->getLastWebResponse( replyMsg );
		LOG4CXX_INFO(statusUpdateRequestLogger, user << "StatusUpdateRequest response " << replyMsg );
	}
}

void StatusUpdateRequest::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(statusUpdateRequestLogger, user << " - Curl error: " << curlerror);
		callBack(user, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) {
			LOG4CXX_ERROR(statusUpdateRequestLogger, user << " - " << error.getMessage());
		} else {
			LOG4CXX_INFO(statusUpdateRequestLogger, "Updated status for " << user << ": " << data);
		}
		callBack(user, error);
	}
}
