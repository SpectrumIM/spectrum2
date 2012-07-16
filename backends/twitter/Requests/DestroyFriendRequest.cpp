#include "DestroyFriendRequest.h"
DEFINE_LOGGER(logger, "DestroyFriendRequest")

void DestroyFriendRequest::run()
{
	replyMsg = "";
	success = twitObj->friendshipDestroy(frnd, false);
	if(success) twitObj->getLastWebResponse(replyMsg);
}

void DestroyFriendRequest::finalize()
{
	if(!success) {
		std::string error;
		twitObj->getLastCurlError(error);
		LOG4CXX_ERROR(logger, user << " Curl error: " << error)
		callBack(user, frnd, error);
	} else {
		std::string error;
		error = getErrorMessage(replyMsg);
		if(error.length()) LOG4CXX_ERROR(logger, user << " - " << error)
		callBack(user, frnd, error);
	}
}
