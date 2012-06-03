#include "StatusUpdateRequest.h"
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
		LOG4CXX_INFO(logger, "Updated status for " << user << ": " << data);
	} else {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << "Error - " << replyMsg );
	}
	return;
}
