#include "StatusUpdateRequest.h"
DEFINE_LOGGER(logger, "StatusUpdateRequest")
void StatusUpdateRequest::run() 
{
	if( twitObj.statusUpdate( data ) ) {
		replyMsg = "";
		while(replyMsg.length() == 0) {
			twitObj.getLastWebResponse( replyMsg );
		}
		LOG4CXX_INFO(logger, user << "StatusUpdateRequest response " << replyMsg );
	} else {
		twitObj.getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << "Error - " << replyMsg );
	}
}

void StatusUpdateRequest::finalize()
{
	LOG4CXX_INFO(logger, "Updated status for " << user << ": " << data);
	return;
}
