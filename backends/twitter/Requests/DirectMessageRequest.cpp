#include "DirectMessageRequest.h"
#include "../TwitterResponseParser.h"

DEFINE_LOGGER(logger, "DirectMessageRequest")

void DirectMessageRequest::run() 
{
	replyMsg = "";
	if(twitObj->directMessageSend(username, data, false) == false) {
		LOG4CXX_ERROR(logger, user << ": Error while sending directed message to " << username );
		return;
	}
	twitObj->getLastWebResponse( replyMsg );
}

void DirectMessageRequest::finalize()
{
	if(replyMsg.length()) {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) {
			np->handleMessage(user, "twitter-account", error);
			LOG4CXX_INFO(logger, user << ": " << error);
		} else {
			LOG4CXX_INFO(logger, user << ": Sent " << data << " to " << username)
			LOG4CXX_INFO(logger, user << ": Twitter reponse - " << replyMsg)
		}
	}
}
