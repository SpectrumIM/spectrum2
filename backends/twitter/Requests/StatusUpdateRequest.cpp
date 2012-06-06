#include "StatusUpdateRequest.h"
#include "../TwitterResponseParser.h"

DEFINE_LOGGER(logger, "StatusUpdateRequest")
void StatusUpdateRequest::run() 
{
	replyMsg = "";
	if( twitObj->statusUpdate( data ) ) {
		while(replyMsg.length() == 0) {
			twitObj->getLastWebResponse( replyMsg );
		}
		LOG4CXX_INFO(logger, user << "StatusUpdateRequest response " << replyMsg );
	} 
}

void StatusUpdateRequest::finalize()
{
	if(replyMsg != "" ) {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) {
			np->handleMessage(user, "twitter-account", error);
			LOG4CXX_INFO(logger, user << ": " << error);
		} else	LOG4CXX_INFO(logger, "Updated status for " << user << ": " << data);
	} else {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << ": CurlError - " << replyMsg );
	}
	return;
}
