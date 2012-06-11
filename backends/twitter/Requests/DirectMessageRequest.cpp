#include "DirectMessageRequest.h"

DEFINE_LOGGER(logger, "DirectMessageRequest")

void DirectMessageRequest::run() 
{
	replyMsg = "";
	if(username != "") success = twitObj->directMessageSend(username, data, false);
	else success = twitObj->directMessageGet(data); /* data will contain sinceId */

	if(success) {
		twitObj->getLastWebResponse( replyMsg );
		if(username == "" ) messages = getDirectMessages( replyMsg );
	}
}

void DirectMessageRequest::finalize()
{
	if(!success) {
		LOG4CXX_ERROR(logger, user << ": Error while sending directed message to " << username );
		twitObj->getLastCurlError( replyMsg );
		callBack(user, messages, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		else LOG4CXX_INFO(logger, user << " - " << replyMsg)
		callBack(user, messages, error);	
	}
}
