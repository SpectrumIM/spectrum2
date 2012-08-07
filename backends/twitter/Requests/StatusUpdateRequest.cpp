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
	if(!success) {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << " - Curl error: " << replyMsg );
		callBack(user, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger, user << " - " << error)
		else LOG4CXX_INFO(logger, "Updated status for " << user << ": " << data);
		callBack(user, error);
	}
}
