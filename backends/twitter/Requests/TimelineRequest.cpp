#include "TimelineRequest.h"
DEFINE_LOGGER(logger, "TimelineRequest")
void TimelineRequest::run()
{	
		
	if(userRequested != "") success = twitObj->timelineUserGet(false, false, 20, userRequested, false);
	else success = twitObj->timelineHomeGet(since_id);
	
	replyMsg = ""; 
	if(success) {	
		LOG4CXX_INFO(logger, "Sending timeline request for user " << user)

		while(replyMsg.length() == 0) {
			twitObj->getLastWebResponse( replyMsg );
		}

		LOG4CXX_INFO(logger, user << " - " << replyMsg.length() << " " << replyMsg << "\n" );
		
		std::vector<Status> tweets = getTimeline(replyMsg);
		timeline = "\n";

		if(tweets.size() && (since_id == "" || 
							 (since_id != "" && tweets[0].getID() != np->getMostRecentTweetID(user)) )) {
			for(int i=0 ; i<tweets.size() ; i++) {
				timeline += tweets[i].getTweet() + "\n";
			}
			np->updateUsersLastTweetID(user, tweets[0].getID());
		}
	}
}

void TimelineRequest::finalize()
{
	if(success && timeline != "\n") {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) {
			np->handleMessage(user, "twitter-account", error);
			LOG4CXX_INFO(logger, user << ": " << error);
		} else {	
			LOG4CXX_INFO(logger, user << "'s timeline\n" << replyMsg);
			np->handleMessage(user, "twitter-account", timeline); //send timeline
		}
	}
	else {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << " - " << replyMsg );
	}
}
