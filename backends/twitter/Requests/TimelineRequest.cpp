#include "TimelineRequest.h"
DEFINE_LOGGER(logger, "TimelineRequest")
void TimelineRequest::run()
{	
	LOG4CXX_INFO(logger, "Sending timeline request for user " << userRequested)
	
	if(userRequested != "") success = twitObj->timelineUserGet(false, false, 20, userRequested, false);
	else success = twitObj->timelineHomeGet(since_id);
	
	if(!success) return;
	
	replyMsg = ""; 
	twitObj->getLastWebResponse( replyMsg );
	//LOG4CXX_INFO(logger, user << " - " << replyMsg.length() << " " << replyMsg << "\n" );
	tweets = getTimeline(replyMsg);
}

void TimelineRequest::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);	
		LOG4CXX_ERROR(logger,  user << " - Curl error: " << curlerror)
		callBack(user, userRequested, tweets, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) LOG4CXX_ERROR(logger,  user << " - " << error.getMessage())
		callBack(user, userRequested, tweets, error);
	} 
}
