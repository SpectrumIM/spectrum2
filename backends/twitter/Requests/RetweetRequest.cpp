#include "RetweetRequest.h"
DEFINE_LOGGER(logger, "RetweetRequest")
void RetweetRequest::run()
{
	LOG4CXX_INFO(logger, user << " Retweeting " << data)
	success = twitObj->retweetById( data );
}

void RetweetRequest::finalize()
{
	Error error;
	if(!success) {
		std::string curlerror;
		twitObj->getLastCurlError(curlerror);
		error.setMessage(curlerror);	
		LOG4CXX_ERROR(logger, user << " Curl error: " << curlerror)
		callBack(user, error);
	} else {
		twitObj->getLastWebResponse(replyMsg);
		error = getErrorMessage(replyMsg);
		if(error.getMessage().length()) LOG4CXX_ERROR(logger, user << " - " << error.getMessage())
		else LOG4CXX_INFO(logger, user << " " << replyMsg);
		callBack(user, error);
	}
}
