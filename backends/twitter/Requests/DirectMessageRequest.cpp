#include "DirectMessageRequest.h"
#include "../TwitterResponseParser.h"

DEFINE_LOGGER(logger, "DirectMessageRequest")

void DirectMessageRequest::run() 
{
	replyMsg = "";
	success = twitObj->directMessageSend(username, data, false);
	if(success) twitObj->getLastWebResponse( replyMsg );
}

void DirectMessageRequest::finalize()
{
	if(!success) {
		LOG4CXX_ERROR(logger, user << ": Error while sending directed message to " << username );
		twitObj->getLastCurlError( replyMsg );
		callBack(user, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		else LOG4CXX_INFO(logger, user << " - " << replyMsg)
		callBack(user, error);	
	}
}
