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

	LOG4CXX_INFO(logger, user << " - " << replyMsg.length() << " " << replyMsg << "\n" );
	
	tweets = getTimeline(replyMsg);
}

void TimelineRequest::finalize()
{
	if(!success) {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger,  user << " - " << replyMsg)
		callBack(user, userRequested, tweets, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		callBack(user, userRequested, tweets, error);
	} 
}
