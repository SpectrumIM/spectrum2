#include "TimelineRequest.h"
DEFINE_LOGGER(logger, "TimelineRequest")
void TimelineRequest::run()
{	
	replyMsg = ""; 
	if( twitObj.timelinePublicGet() ) {	
		LOG4CXX_INFO(logger, "Sending timeline request for user " << user)

		while(replyMsg.length() == 0) {
			twitObj.getLastWebResponse( replyMsg );
		}

		LOG4CXX_INFO(logger, user << " - " << replyMsg.length() << " " << replyMsg << "\n" );
		
		std::vector<Status> tweets = getTimeline(replyMsg);
		timeline = "\n";
		for(int i=0 ; i<tweets.size() ; i++) {
			timeline += tweets[i].getTweet() + "\n";
		}
	}
}

void TimelineRequest::finalize()
{
	if(replyMsg.length()) {
		LOG4CXX_INFO(logger, user << "'s timeline\n" << replyMsg);
		np->handleMessage(user, "twitter-account", timeline); //send timeline
	}
	else {
		twitObj.getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << " - " << replyMsg );
	}
}
