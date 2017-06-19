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
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);
		LOG4CXX_ERROR(logger, user << " Curl error: " << curlerror);
		callBack(user, username, messages, error);
	} else {
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) {
			LOG4CXX_ERROR(logger,  user << " - " << error.getMessage());
		} else {
			LOG4CXX_INFO(logger, user << " - " << replyMsg);
		}
		callBack(user, username, messages, error);
	}
}
