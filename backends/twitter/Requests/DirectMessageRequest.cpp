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
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << " Curl error: " << replyMsg);
		callBack(user, username, messages, replyMsg);
	} else {
		std::string error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger,  user << " - " << error)
		else LOG4CXX_INFO(logger, user << " - " << replyMsg)
		callBack(user, username, messages, error);	
	}
}
