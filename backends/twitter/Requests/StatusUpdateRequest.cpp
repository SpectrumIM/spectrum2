#include "StatusUpdateRequest.h"
#include "../TwitterResponseParser.h"

DEFINE_LOGGER(logger, "StatusUpdateRequest")
void StatusUpdateRequest::run()
{
	replyMsg = "";
	success = twitObj->statusUpdate(data);
	if(success) {
		twitObj->getLastWebResponse( replyMsg );
		LOG4CXX_INFO(logger, user << "StatusUpdateRequest response " << replyMsg );
	}
}

void StatusUpdateRequest::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(logger, user << " - Curl error: " << curlerror);
		callBack(user, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) LOG4CXX_ERROR(logger, user << " - " << error.getMessage());
		else LOG4CXX_INFO(logger, "Updated status for " << user << ": " << data);
		callBack(user, error);
	}
}
