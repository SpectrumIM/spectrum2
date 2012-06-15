#include "RetweetRequest.h"
DEFINE_LOGGER(logger, "RetweetRequest")
void RetweetRequest::run()
{
	LOG4CXX_INFO(logger, user << " Retweeting " << data)
	success = twitObj->retweetById( data );
}

void RetweetRequest::finalize()
{
	replyMsg = "";
	if(!success) {
		twitObj->getLastCurlError( replyMsg );
		LOG4CXX_ERROR(logger, user << " " << replyMsg)
		callBack(user, replyMsg);
	} else {
		twitObj->getLastWebResponse( replyMsg );
		std::string error = getErrorMessage( replyMsg );
		if(error.length()) LOG4CXX_ERROR(logger, user << " " << error)
		else LOG4CXX_INFO(logger, user << " " << replyMsg);
		callBack(user, error);
	}
}
